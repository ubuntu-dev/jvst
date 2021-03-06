/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef JVST_AST_H
#define JVST_AST_H

#include "jdom.h"

#include <stdio.h>

#include <re/re.h>

struct fsm;

typedef json_number ast_number;
typedef unsigned long ast_count;

struct ast_regexp {
        enum re_dialect dialect;
	struct json_string str;
	struct fsm *fsm;
};

/* unordered set, heterogenous */
struct ast_value_set {
	struct json_value value;
	struct ast_value_set *next;
};

/* unordered set */
struct ast_schema_set {
	struct ast_schema *schema;
	struct ast_schema_set *next;
};

/* unordered unique strings */
struct ast_string_set {
	struct json_string str;
	struct ast_string_set *next;
};

/* ordered array */
struct ast_schema_array {
	struct ast_schema *schema;
	struct ast_schema_array *next;
};

/* unordered k/v set */
struct ast_property_schema {
	struct ast_regexp pattern;
	struct ast_schema *schema;
	struct ast_property_schema *next;
};

/* unordered k/v set */
struct ast_property_names {
	struct ast_regexp pattern;
	struct ast_string_set *set;
	struct ast_property_names *next;
};

struct ast_schema {
	/*
	 * Multiple occurances of a keyword override the previous instance,
	 * according to ajv.
	 *
	 * The .kws bitmap indicates presence for keywords which are set
	 * (i.e. that their corresponding fields have meaningful values).
	 *
	 * Some keywords are not present in this bitmap, because their
	 * corresponding fields carry their own information to indicate
	 * whether their values are meaningful or not. These are:
	 *
	 * "items":                          .items is NULL
	 * "additionalItems":                .additional_items is NULL
	 * "uniqueItems":                    .unique_items defaults false
	 * "contains":                       .contains is NULL
	 * "required":                       .required.set is NULL
	 * "pattern":                        .pattern.str NULL
	 *
	 * "properties"/"patternProperties": .properties.set is NULL
	 * "additionalProperties":           .additional_properties.set is NULL
	 *
	 * "dependencies":                   .dependencies_strings/schema.set are NULL
	 *
	 * "propertyNames":                  .property_names is NULL
	 * "enum"/"const":                   .xenum is NULL
	 * "type":                           .type is 0
	 *
	 * "anyOf"/"allOf"/"oneOf":          .some_of.set is NULL
	 * "not":                            .not is NULL
	 * "title":                          .title.str is NULL
	 * "description":                    .description.str is NULL
	 * "$Id":                            .id.str is NULL
	 * "$Ref":                           unioned during parse
	 *
	 * "default":  TODO: unimplemented
	 * "examples": TODO: unimplemented
	 */
	enum ast_kws {
		KWS_VALUE                 = 1 <<  0,

		KWS_MULTIPLE_OF           = 1 <<  1,
		KWS_MAXIMUM               = 1 <<  2, /* also "exclusiveMaximum" */
		KWS_MINIMUM               = 1 <<  3, /* also "exclusiveMinimum" */

		KWS_MIN_LENGTH            = 1 <<  5, /* .min_length and .max_length */
		KWS_MAX_LENGTH            = 1 <<  6, /* .min_length and .max_length */

		KWS_MIN_ITEMS             = 1 <<  7, /* .min_items  and .max_items */
		KWS_MAX_ITEMS             = 1 <<  8, /* .min_items  and .max_items */

		KWS_MIN_PROPERTIES        = 1 <<  9, /* .min_properties and .max_properties */
		KWS_MAX_PROPERTIES        = 1 << 10, /* .min_properties and .max_properties */

		KWS_SINGLETON_ITEMS	  = 1 << 11, /* .items was not an array */

		KWS_HAS_REF		  = 1 << 12, /* .ref, must ignore everything else */
	} kws;

	struct json_string ref;

	/* TODO: transform post-parse to populate AST_STRING to .pattern instead */
	struct json_value value;

	ast_number multiple_of; /* > 0 */

	/* TODO: confirm that "exclusiveMaximum" overrides "maximum" rather than both applying */
	bool exclusive_maximum; /* .maximum is "exclusiveMaximum" rather than "maximum" */
	bool exclusive_minimum; /* .minimum is "exclusiveMinimum" rather than "minimum" */
	ast_number maximum;
	ast_number minimum;

	struct ast_regexp pattern;

	/* min/max fields are only valid when appropriate KWS_* flags
	 * are set.  Otherwise the defaults are: min is 0, max is unbounded
	 */
	ast_count max_length;
	ast_count min_length;
	ast_count max_items;
	ast_count min_items;
	ast_count max_properties;
	ast_count min_properties;

	struct ast_schema_set *items; /* 1 or more; empty means absent */
	struct ast_schema *additional_items;

	bool unique_items; /* defaults false */

	struct ast_schema *contains;

	struct {
		struct ast_string_set *set;
		struct fsm *fsm; /* union to one dfa */
	} required;

	/*
	 * "properties":           keyed by string literal
	 * "patternProperties":    keyed by regexp
	 */
	struct {
		struct ast_property_schema *set;
		struct fsm *fsm; /* union to one dfa */
	} properties;

	/*
	 * "additionalProperties": schema set
	 */
	struct ast_schema *additional_properties;

	/* "dependencies": array form */
	struct {
		struct ast_property_names *set;
		struct fsm *fsm; /* union to one dfa */
	} dependencies_strings;

	/* "dependencies": schema form */
	struct {
		struct ast_property_schema *set;
		struct fsm *fsm; /* union to one dfa */
	} dependencies_schema;

	/* TODO: union DFA from .required, .properties, .additional_properties,
	 * .dependencies_strings/schema
	 * to map to opaque pointers which disambiguate which is responsible */

	struct ast_schema *property_names;

	/*
	 * "enum":  an unordered set
	 * "const": an enum of a single item
	 */
	struct ast_value_set *xenum;

	enum json_valuetype types; /* bitmap; 0 for unset */

	/*
	 * "allOf": min = n, max = n
	 * "anyOf": min = 1, max = n
	 * "oneOf": min = 1, max = 1
	 */
	struct {
		unsigned min;
		unsigned max;
		struct ast_schema_set *set; /* non-empty */
	} some_of;

	struct ast_schema *not;

	struct ast_schema_set *definitions;

	struct json_string schema;
	struct ast_string_set *all_ids;
	struct json_string title;
	struct json_string description;

	struct json_value *xdefault;
	struct ast_value_set *examples;

	struct ast_schema *next;
};

void
ast_dump(FILE *f, const struct ast_schema *ast);

#endif

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
