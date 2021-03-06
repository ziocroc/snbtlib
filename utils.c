/*
 * Copyright (c) 2013, ZioCrocifisso
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

#include "nbt.h"

static nbt_tag *next_tag(nbt_tag *tag);
static nbt_payload *next_payload(nbt_tag *tag);
static int insert(nbt_tag *tag, void *value);
static int delete(nbt_tag *tag);

static int set_string(nbt_string *payload, char *value);

nbt_tag *nbt_new(void)
{
	nbt_tag *tag = malloc(sizeof(nbt_tag));

	if (tag) {
		tag->id = NBT_TAG_END;
		tag->name.length = 0;
		tag->name.chars = (char *) 0;
	}
	
	return tag;
}

nbt_byte nbt_get_type(nbt_tag *tag)
{
	return tag ? tag->id : NBT_TAG_INVALID;
}

char *nbt_get_name(nbt_tag *tag)
{
	return tag ? tag->name.chars : (char *) NBT_ERR_INVALID_ARG;
}

size_t nbt_get_length(nbt_tag *tag)
{
	nbt_tag *t;
	int l = 0;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	switch (tag->id) {
		case NBT_TAG_BYTE_ARRAY:
			return tag->payload.byte_array_payload.size;

		case NBT_TAG_LIST:
			return tag->payload.list_payload.size;

		case NBT_TAG_COMPOUND:
			if (tag->payload.compound_payload == (nbt_tag *) 0) {
				return 0;
			}

			for (t = tag->payload.compound_payload; t->id != NBT_TAG_END; t++) {
				l++;
			}

			return l;

		case NBT_TAG_INT_ARRAY:
			return tag->payload.int_array_payload.size;

		case NBT_TAG_STRING:
			return tag->payload.string_payload.length;

		default:
			return 1;
	}
}

nbt_byte nbt_get_children_type(nbt_tag *tag)
{
	if (!tag) {
		return NBT_TAG_INVALID;
	}

	switch (tag->id) {
		case NBT_TAG_BYTE_ARRAY:
			return NBT_TAG_BYTE;

		case NBT_TAG_INT_ARRAY:
			return NBT_TAG_INT;

		case NBT_TAG_LIST:
			return tag->payload.list_payload.tag_id;

		case NBT_TAG_COMPOUND:
			return NBT_TAG_TAG;

		default:
			return tag->id;
	}
}

int64_t nbt_get_integer(nbt_tag *tag)
{
	nbt_payload *payload;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	payload = next_payload(tag);

	switch (tag->id) {
		case NBT_TAG_BYTE:
			return (int64_t) payload->byte_payload;

		case NBT_TAG_SHORT:
			return (int64_t) payload->short_payload;

		case NBT_TAG_INT:
			return (int64_t) payload->int_payload;

		case NBT_TAG_LONG:
			return (int64_t) payload->long_payload;

		default:
			return 0;
	}
}

double nbt_get_real(nbt_tag *tag)
{
	nbt_payload *payload;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	payload = next_payload(tag);

	switch (tag->id) {
		case NBT_TAG_FLOAT:
			return (double) payload->float_payload;

		case NBT_TAG_DOUBLE:
			return (double) payload->double_payload;

		default:
			return 0;
	}
}

char *nbt_get_string(nbt_tag *tag)
{
	nbt_payload *payload;

	if (!tag) {
		return (char *) 0;
	}

	payload = next_payload(tag);

	return payload->string_payload.chars;
}

nbt_tag *nbt_get_tag(nbt_tag *tag)
{
	if (!tag) {
		return (nbt_tag *) 0;
	}

	if (tag->id != NBT_TAG_COMPOUND) {
		return (nbt_tag *) 0;
	}

	return next_tag(tag);
}

nbt_tag nbt_get_multiple(nbt_tag *tag)
{
	nbt_payload *p;
	nbt_tag fake_tag = {
		NBT_TAG_INVALID,	
		{ 0 },
		0,
		(nbt_string) {
			0,
			(char *) 0
		}
	};

	if (!tag || !NBT_IS_PARENT(tag->id)) {
		return fake_tag;
	}

	fake_tag.id = nbt_get_children_type(tag);
	fake_tag.position = 0;
	p = next_payload(tag);

	if (p) {
		fake_tag.payload = *p;
	} else {
		fake_tag.id = NBT_TAG_INVALID;
	}

	return fake_tag;
}

int nbt_set_position(nbt_tag *tag, uint64_t value)
{
	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	return (tag->position = value);
}

int nbt_set_type(nbt_tag *tag, nbt_byte value)
{
	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	if (value < NBT_TAG_END || value > NBT_TAG_INT_ARRAY) {
		return NBT_ERR_INVALID_ARG;
	}

	tag->id = value;
	return 1;
}

int nbt_set_name(nbt_tag *tag, char *value)
{
	if (!tag || !value) {
		return NBT_ERR_INVALID_ARG;
	}

	return set_string(&tag->name, value);
}

int nbt_set_children_type(nbt_tag *tag, nbt_byte value)
{
	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	if (tag->id == NBT_TAG_BYTE_ARRAY || tag->id == NBT_TAG_INT_ARRAY) {
		tag->id = NBT_TAG_LIST;
		tag->payload.list_payload.tag_id = value;
		tag->payload.list_payload.size = 0;
		tag->payload.list_payload.tags = (nbt_payload *) 0;

		return 1;
	}

	if (tag->id != NBT_TAG_LIST) {
		return NBT_ERR_INVALID_TYPE;
	}

	if (nbt_get_length(tag) <= 0) {
		tag->payload.list_payload.tag_id = value;

		return 1;
	}

	return NBT_ERR_NONEMPTY;
}

int nbt_set_integer(nbt_tag *tag, int64_t value)
{
	nbt_payload *p;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	if (!NBT_IS_INTEGER(nbt_get_children_type(tag))) {
		return NBT_ERR_INVALID_TYPE;
	}

	p = next_payload(tag);

	p->long_payload = value;
	return 1;
}

int nbt_set_real(nbt_tag *tag, double value)
{
	nbt_payload *p;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	if (!NBT_IS_REAL(nbt_get_children_type(tag))) {
		return NBT_ERR_INVALID_TYPE;
	}

	p = next_payload(tag);

	if (tag->id == NBT_TAG_FLOAT) {
		p->float_payload = (float) value;
	} else {
		p->double_payload = value;
	}

	return 1;
}

int nbt_set_string(nbt_tag *tag, char *value)
{
	nbt_payload *p;

	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	if (!NBT_IS_STRING(nbt_get_children_type(tag))) {
		return NBT_ERR_INVALID_TYPE;
	}

	p = next_payload(tag);
	return set_string(&p->string_payload, value);
}

int nbt_insert_integer(nbt_tag *tag, int64_t value)
{
	if (!tag || !value) {
		return NBT_ERR_INVALID_ARG;
	}

	return insert(tag, (nbt_payload *) &value);
}

int nbt_insert_real(nbt_tag *tag, double value)
{
	if (!tag || !value) {
		return NBT_ERR_INVALID_ARG;
	}

	return insert(tag, (nbt_payload *) &value);
}

int nbt_insert_string(nbt_tag *tag, char *value)
{
	nbt_string payload;

	if (!tag || !value) {
		return NBT_ERR_INVALID_ARG;
	}

	set_string(&payload, value);

	return insert(tag, (nbt_payload *) &payload);
}

int nbt_insert_tag(nbt_tag *tag, nbt_tag *value)
{
	if (!tag || !value) {
		return NBT_ERR_INVALID_ARG;
	}

	return insert(tag, value);
}

int nbt_remove(nbt_tag *tag)
{
	if (!tag) {
		return NBT_ERR_INVALID_ARG;
	}

	return delete(tag);
}

void nbt_free(nbt_tag *tag)
{
	nbt_tag *t;

	if (!tag) {
		return;
	}

	nbt_free_children(tag);

	free(tag);
}

void nbt_free_children(nbt_tag *tag)
{
	nbt_tag *t;

	if (!tag) {
		return;
	}

	if (tag->name.chars) {
		free(tag->name.chars);
	}

	switch (tag->id) {
		case NBT_TAG_BYTE_ARRAY:
			if (tag->payload.byte_array_payload.bytes) {
				free(tag->payload.byte_array_payload.bytes);
			}

			break;

		case NBT_TAG_STRING:
			if (tag->payload.string_payload.chars) {
				free(tag->payload.string_payload.chars);
			}

			break;

		case NBT_TAG_LIST:
			if (tag->payload.list_payload.tags) {
				free(tag->payload.list_payload.tags);
			}

			break;

		case NBT_TAG_COMPOUND:
			for (t = tag->payload.compound_payload; t->id != NBT_TAG_END; t++) {
				nbt_free_children(t);
			}

			free(tag->payload.compound_payload);

			break;

		case NBT_TAG_INT_ARRAY:
			if (tag->payload.int_array_payload.ints) {
				free(tag->payload.int_array_payload.ints);
			}

			break;
	}
}

static nbt_tag *next_tag(nbt_tag *tag)
{
	if (tag->position >= nbt_get_length(tag)) {
		tag->position = 0;
	}

	return tag->payload.compound_payload + tag->position++;
}

static nbt_payload *next_payload(nbt_tag *tag)
{
	if (tag->position >= nbt_get_length(tag)) {
		tag->position = 0;
	}

	switch (tag->id) {
		case NBT_TAG_BYTE_ARRAY:
			return (nbt_payload *) &tag->payload.byte_array_payload.bytes[tag->position++];

		case NBT_TAG_LIST:
			return &tag->payload.list_payload.tags[tag->position++];

		case NBT_TAG_COMPOUND:
			return &tag->payload.compound_payload[tag->position++].payload;

		case NBT_TAG_INT_ARRAY:
			return (nbt_payload *) &tag->payload.int_array_payload.ints[tag->position++];

		default:
			return &tag->payload;
	}
}

#define GET_MUL_ATTRIBUTES									\
	switch (tag->id) {									\
		case NBT_TAG_BYTE_ARRAY:							\
			unit_size = sizeof(nbt_byte);						\
			origp = (void *) &tag->payload.byte_array_payload.bytes;		\
			lenout = &tag->payload.byte_array_payload.size;				\
			length = *lenout;							\
			break;									\
												\
		case NBT_TAG_LIST:								\
			unit_size = sizeof(nbt_payload);					\
			origp = (void *) &tag->payload.list_payload.tags;			\
			lenout =  &tag->payload.list_payload.size;				\
			length = *lenout;							\
			break;									\
												\
		case NBT_TAG_INT_ARRAY:								\
			unit_size = sizeof(nbt_int);						\
			origp = (void *) &tag->payload.int_array_payload.ints;			\
			lenout = &tag->payload.int_array_payload.size;				\
			length = *lenout;							\
			break;									\
												\
		case NBT_TAG_COMPOUND:								\
			unit_size = sizeof(nbt_tag);						\
			origp = (void *) &tag->payload.compound_payload;			\
			length = nbt_get_length(tag);						\
			break;									\
												\
		default:									\
			return NBT_ERR_INVALID_TYPE;						\
	}											

static int insert(nbt_tag *tag, void *value)
{
	size_t unit_size, length;
	nbt_int *lenout = (nbt_int *) 0;
	void **origp;
	void *copy;
	int elem_pos;

	GET_MUL_ATTRIBUTES

	copy = malloc(++length * unit_size);

	if (!copy)  {
		return NBT_ERR_MEMORY;
	}

	elem_pos = unit_size * tag->position;

	memcpy(copy, *origp, elem_pos);
	memcpy(copy + elem_pos, value, unit_size);
	memcpy(copy + elem_pos + unit_size, *origp + elem_pos, (length - 1) * unit_size - elem_pos);

	if (*origp) {
		free(*origp);
	}

	*origp = copy;

	if (lenout) {
		*lenout = length;
	}
	tag->position++;

	return 1;
}

static int delete(nbt_tag *tag)
{
	void *copy, **origp;
	int elem_pos;
	size_t unit_size, length;
	nbt_int *lenout = (nbt_int *) 0;

	GET_MUL_ATTRIBUTES

	if (length == 0) {
		return 1;
	}

	elem_pos = unit_size * tag->position;

	if (tag->id == NBT_TAG_COMPOUND) {
		nbt_free((nbt_tag *) (*origp + elem_pos));
	}

	memmove(
			*origp + elem_pos,
			*origp + elem_pos + unit_size,
			(length * unit_size) - (elem_pos + unit_size)
	);

	copy = realloc(*origp, --length * unit_size);

	if (!copy) {
		return NBT_ERR_MEMORY;
	}

	*origp = copy;

	if (lenout) {
		*lenout = length;
	}

	if (tag->position) {
		tag->position--;
	}

	return 1;
}


static int set_string(nbt_string *payload, char *value)
{
	char *newchars;
	int len;

	len = strlen(value);
	newchars = (char *) realloc(payload->chars, len);

	if (!newchars) {
		return NBT_ERR_MEMORY;
	}

	memcpy(newchars, value, len);

	payload->chars = newchars;
	payload->length = len;

	return 1;
}
