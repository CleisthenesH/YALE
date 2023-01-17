// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "hash.h"
#include <stdlib.h>
#include <string.h>

/* There is a problem with this implimentation.
 * Because of how function names are written we don't actually have as much varity as it might look, making a singular matrix likely:
 * 
 *		set_keyframe_abc
 *		set_keyframe_def
 *		set_keyframe_ghi
 *	
 * Only have three characters of variantion between them.
 * This makes a row being a linear combination of another likey, which makes a singular matrix likey.
 * 
 * Consider having two hash functions.
 * One called "encode" which takes from the source set to an n sized block of uint8_t.
 * One that takes an block to a single uint8_t.
 * 
 * The encode has to sensitvly depend on every input char and can have a random offset to change to force a non-singular matrix.
 * (This random number might be called a nonce or salt in proper cryptographic circles, not sure.
 */

// F = GF(2)
// F[x]/(x^8 + x^4 + x^3 + x + 1)
/* Add two numbers in the GF(2^8) finite field */
// Rijndael field

// modulo polynomial relation x^8 + x^4 + x^3 + x + 1 = 0

// power[x] = (1+x)^n
// Haveing power[0] lets us save checkng for 1 when writing the invert function.
static const uint8_t power[] = {
	0x01, 0x03, 0x05, 0x0F, 0x11, 0x33, 0x55, 0xFF, 0x1A, 0x2E, 0x72, 0x96, 0xA1, 0xF8, 0x13, 0x35,
	0x5F, 0xE1, 0x38, 0x48, 0xD8, 0x73, 0x95, 0xA4, 0xF7, 0x02, 0x06, 0x0A, 0x1E, 0x22, 0x66, 0xAA,
	0xE5, 0x34, 0x5C, 0xE4, 0x37, 0x59, 0xEB, 0x26, 0x6A, 0xBE, 0xD9, 0x70, 0x90, 0xAB, 0xE6, 0x31,
	0x53, 0xF5, 0x04, 0x0C, 0x14, 0x3C, 0x44, 0xCC, 0x4F, 0xD1, 0x68, 0xB8, 0xD3, 0x6E, 0xB2, 0xCD,
	0x4C, 0xD4, 0x67, 0xA9, 0xE0, 0x3B, 0x4D, 0xD7, 0x62, 0xA6, 0xF1, 0x08, 0x18, 0x28, 0x78, 0x88,
	0x83, 0x9E, 0xB9, 0xD0, 0x6B, 0xBD, 0xDC, 0x7F, 0x81, 0x98, 0xB3, 0xCE, 0x49, 0xDB, 0x76, 0x9A,
	0xB5, 0xC4, 0x57, 0xF9, 0x10, 0x30, 0x50, 0xF0, 0x0B, 0x1D, 0x27, 0x69, 0xBB, 0xD6, 0x61, 0xA3,
	0xFE, 0x19, 0x2B, 0x7D, 0x87, 0x92, 0xAD, 0xEC, 0x2F, 0x71, 0x93, 0xAE, 0xE9, 0x20, 0x60, 0xA0,
	0xFB, 0x16, 0x3A, 0x4E, 0xD2, 0x6D, 0xB7, 0xC2, 0x5D, 0xE7, 0x32, 0x56, 0xFA, 0x15, 0x3F, 0x41,
	0xC3, 0x5E, 0xE2, 0x3D, 0x47, 0xC9, 0x40, 0xC0, 0x5B, 0xED, 0x2C, 0x74, 0x9C, 0xBF, 0xDA, 0x75,
	0x9F, 0xBA, 0xD5, 0x64, 0xAC, 0xEF, 0x2A, 0x7E, 0x82, 0x9D, 0xBC, 0xDF, 0x7A, 0x8E, 0x89, 0x80,
	0x9B, 0xB6, 0xC1, 0x58, 0xE8, 0x23, 0x65, 0xAF, 0xEA, 0x25, 0x6F, 0xB1, 0xC8, 0x43, 0xC5, 0x54,
	0xFC, 0x1F, 0x21, 0x63, 0xA5, 0xF4, 0x07, 0x09, 0x1B, 0x2D, 0x77, 0x99, 0xB0, 0xCB, 0x46, 0xCA,
	0x45, 0xCF, 0x4A, 0xDE, 0x79, 0x8B, 0x86, 0x91, 0xA8, 0xE3, 0x3E, 0x42, 0xC6, 0x51, 0xF3, 0x0E,
	0x12, 0x36, 0x5A, 0xEE, 0x29, 0x7B, 0x8D, 0x8C, 0x8F, 0x8A, 0x85, 0x94, 0xA7, 0xF2, 0x0D, 0x17,
	0x39, 0x4B, 0xDD, 0x7C, 0x84, 0x97, 0xA2, 0xFD, 0x1C, 0x24, 0x6C, 0xB4, 0xC7, 0x52, 0xF6, 0x01,
};

// n = (1+x)^index[n]
// index[0] is meaningless padding and should never be accessed.
static const uint8_t index[] = {
	0x00, 0xFF, 0x19, 0x01, 0x32, 0x02, 0x1A, 0xC6, 0x4B, 0xC7, 0x1B, 0x68, 0x33, 0xEE, 0xDF, 0x03,
	0x64, 0x04, 0xE0, 0x0E, 0x34, 0x8D, 0x81, 0xEF, 0x4C, 0x71, 0x08, 0xC8, 0xF8, 0x69, 0x1C, 0xC1,
	0x7D, 0xC2, 0x1D, 0xB5, 0xF9, 0xB9, 0x27, 0x6A, 0x4D, 0xE4, 0xA6, 0x72, 0x9A, 0xC9, 0x09, 0x78,
	0x65, 0x2F, 0x8A, 0x05, 0x21, 0x0F, 0xE1, 0x24, 0x12, 0xF0, 0x82, 0x45, 0x35, 0x93, 0xDA, 0x8E,
	0x96, 0x8F, 0xDB, 0xBD, 0x36, 0xD0, 0xCE, 0x94, 0x13, 0x5C, 0xD2, 0xF1, 0x40, 0x46, 0x83, 0x38,
	0x66, 0xDD, 0xFD, 0x30, 0xBF, 0x06, 0x8B, 0x62, 0xB3, 0x25, 0xE2, 0x98, 0x22, 0x88, 0x91, 0x10,
	0x7E, 0x6E, 0x48, 0xC3, 0xA3, 0xB6, 0x1E, 0x42, 0x3A, 0x6B, 0x28, 0x54, 0xFA, 0x85, 0x3D, 0xBA,
	0x2B, 0x79, 0x0A, 0x15, 0x9B, 0x9F, 0x5E, 0xCA, 0x4E, 0xD4, 0xAC, 0xE5, 0xF3, 0x73, 0xA7, 0x57,
	0xAF, 0x58, 0xA8, 0x50, 0xF4, 0xEA, 0xD6, 0x74, 0x4F, 0xAE, 0xE9, 0xD5, 0xE7, 0xE6, 0xAD, 0xE8,
	0x2C, 0xD7, 0x75, 0x7A, 0xEB, 0x16, 0x0B, 0xF5, 0x59, 0xCB, 0x5F, 0xB0, 0x9C, 0xA9, 0x51, 0xA0,
	0x7F, 0x0C, 0xF6, 0x6F, 0x17, 0xC4, 0x49, 0xEC, 0xD8, 0x43, 0x1F, 0x2D, 0xA4, 0x76, 0x7B, 0xB7,
	0xCC, 0xBB, 0x3E, 0x5A, 0xFB, 0x60, 0xB1, 0x86, 0x3B, 0x52, 0xA1, 0x6C, 0xAA, 0x55, 0x29, 0x9D,
	0x97, 0xB2, 0x87, 0x90, 0x61, 0xBE, 0xDC, 0xFC, 0xBC, 0x95, 0xCF, 0xCD, 0x37, 0x3F, 0x5B, 0xD1,
	0x53, 0x39, 0x84, 0x3C, 0x41, 0xA2, 0x6D, 0x47, 0x14, 0x2A, 0x9E, 0x5D, 0x56, 0xF2, 0xD3, 0xAB,
	0x44, 0x11, 0x92, 0xD9, 0x23, 0x20, 0x2E, 0x89, 0xB4, 0x7C, 0xB8, 0x26, 0x77, 0x99, 0xE3, 0xA5,
	0x67, 0x4A, 0xED, 0xDE, 0xC5, 0x31, 0xFE, 0x18, 0x0D, 0x63, 0x8C, 0x80, 0xC0, 0xF7, 0x70, 0x07
};

static uint8_t inline encode_character(char c)
{
	return (uint8_t)c;
}

static void inline encode_block(const char* src, uint8_t* dest, size_t length)
{
	if (!src || !dest)
		return;

	size_t idx = 0;

	for (; idx < length; idx++)
		if (src[idx] != '\0')
			dest[idx] = encode_character(src[idx]);
		else
			break;

	for (; idx < length; idx++)
		dest[idx] = 0;
}

static uint8_t inline gadd(uint8_t a, uint8_t b)
{
	return a ^ b;
}

static uint8_t inline gmul(uint8_t a, uint8_t b)
{
	if (a == 0 || b == 0)
		return 0;

	uint16_t sum = index[a] + index[b];

	if (sum > 255)
		sum -= 255;

	return power[sum];
}

static uint8_t inline ginv(uint8_t a)
{
	return power[index[1] - index[a]];
}

// Matrix operations

struct augmented_matrix
{
	size_t size;
	uint8_t* matrix;
	uint8_t* constants;
};

static void inline augmented_matrix_swap_row(struct augmented_matrix* matrix, size_t i, size_t j)
{
	uint8_t buffer;

	for (size_t k = 0; k < matrix->size; k++)
	{
		buffer = matrix->matrix[matrix->size * i + k];
		matrix->matrix[matrix->size * i + k] = matrix->matrix[matrix->size * j + k];
		matrix->matrix[matrix->size * j + k] = buffer;
	}

	buffer = matrix->constants[i];
	matrix->constants[i] = matrix->constants[j];
	matrix->constants[j] = buffer;
}

// Rearrange the matrix such that all zeros in the ith column i lower than the ith row are moved to the bottom
static size_t inline augmented_matrix_rearrange_zeros(struct augmented_matrix* matrix, size_t i)
{
	size_t leading_zeros = 0;

	for (size_t j = i; j < matrix->size - leading_zeros; j++)
	{
		while (matrix->matrix[i + matrix->size * j] == 0)
		{
			leading_zeros++;

			if (j != matrix->size - leading_zeros)
				augmented_matrix_swap_row(matrix, j, matrix->size - leading_zeros);
			else
				break;
		}
	}

	return leading_zeros;
}

static void inline augmented_matrix_scale_row(struct augmented_matrix* matrix, size_t i, uint8_t scale)
{
	for (size_t k = 0; k < matrix->size; k++)
		matrix->matrix[matrix->size * i + k] = gmul(matrix->matrix[matrix->size * i + k], scale);

	matrix->constants[i] = gmul(matrix->constants[i], scale);
}

// Adds row j to i
static void inline augmented_matrix_add_row(struct augmented_matrix* matrix, size_t i, size_t j)
{
	for (size_t k = 0; k < matrix->size; k++)
		matrix->matrix[matrix->size * i + k] = gadd(matrix->matrix[matrix->size * i + k], matrix->matrix[matrix->size * j + k]);

	matrix->constants[i] = gadd(matrix->constants[i], matrix->constants[j]);
}

// Adds scaled row j to i
static void inline augmented_matrix_scale_and_add_row(struct augmented_matrix* matrix, size_t i, size_t j, uint8_t scale)
{
	uint8_t buffer;

	for (size_t k = 0; k < matrix->size; k++)
	{
		buffer = gmul(scale, matrix->matrix[matrix->size * j + k]);

		matrix->matrix[matrix->size * i + k] = gadd(matrix->matrix[matrix->size * i + k], buffer);
	}

	buffer = gmul(scale, matrix->constants[j]);

	matrix->constants[i] = gadd(matrix->constants[i], buffer);
}

static void inline augmented_matrix_gaussian_elimination(struct augmented_matrix* matrix)
{
	// Make the matrix an upper triangluar with leading zeros
	for (size_t i = 0; i < matrix->size; i++)
	{
		const size_t leading_zeros = augmented_matrix_rearrange_zeros(matrix,i);

	
		const uint8_t leading_val = matrix->matrix[i + matrix->size * i];

		// The ith column is a linear combination of the previous rows so the matrix is singular
		if (leading_val == 0)
		{
			printf("CRITICAL ERROR: matrix is singular\n");
			return;	
		}

		augmented_matrix_scale_row(matrix, i, ginv(leading_val));

		for (size_t j = i + 1; j < matrix->size - leading_zeros; j++)
		{
			const uint8_t inv = ginv(matrix->matrix[i + matrix->size * j]);
			augmented_matrix_scale_row(matrix, j, inv);
			augmented_matrix_add_row(matrix, j, i);
		}
	}

	// Make the matrix the identy matrix
	// This can be optimzed, since we don't care about the matrix at this point there is no reasion to update
	for (size_t i = 1; i < matrix->size; i++)
		for (size_t j = 0; j < i; j++)
			if (matrix->matrix[matrix->size * j + i])
				augmented_matrix_scale_and_add_row(matrix, j, i, matrix->matrix[matrix->size * j + i]);
}

// Actual Hashing

struct hash_data
{
	size_t n;
	uint8_t* coefficients;
};

static size_t get_key_cnt(const char* keys[])
{
	size_t cnt = 0;

	while (keys[cnt++]);

	return --cnt;
}

struct hash_data* calc_hash_data(const char* keys[], uint8_t* values)
{
	const size_t n = get_key_cnt(keys);

	struct augmented_matrix handle = (struct augmented_matrix)
	{
		.size = n,
		.matrix = malloc(n * n * sizeof(uint8_t)),
		.constants = values,
	};

	if (!values)
	{
		handle.constants = malloc(n * sizeof(uint8_t));

		for (size_t i = 0; i < n; i++)
			handle.constants[i] = (uint8_t)i;
	}

	for (size_t i = 0; i < n; i++)
		encode_block(keys[i], handle.matrix + i * n, n);

	augmented_matrix_gaussian_elimination(&handle);

	struct hash_data* output = malloc(sizeof(struct hash_data));
	output->coefficients = malloc(n * sizeof(struct hash_data));

	memcpy(output->coefficients, handle.constants, n * sizeof(uint8_t));
	output->n = n;

	if (!values)
		free(handle.constants);

	free(handle.matrix);

	return output;
}

uint8_t hash(struct hash_data* hash_data, const char* key)
{
	uint8_t output = 0;

	for (size_t i = 0; i < hash_data->n; i++)
		if (key[i] != '\0')
			output ^= gmul(encode_character(key[i]), hash_data->coefficients[i]);
		else
			break;

	return output;
}

// Hash table

struct hash_table
{
	struct hash_data;
	const char*** keys;
};

struct hash_table* hash_table_new(const char*** keys)
{
	struct hash_table* output = malloc(sizeof(struct hash_table));

	struct hash_data* temp = calc_hash_data(keys, NULL);

	*output = (struct hash_table)
	{
		.n = temp->n,
		.coefficients = malloc(sizeof(uint8_t) * temp->n),
	};

	return NULL;
}

uint8_t hash_table_get(struct hash_table* table, const char* key)
{
	return 0;
}
