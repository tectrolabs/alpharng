/**
 *    @file sample_c.c
 *    @date 07/20/2024
 *    @version 1.5
 *
 *    @brief A C example that utilizes a C wrapper around the C++ API for communicating with the AlphaRNG device.
 */
#include <AlphaRngApiCWrapper.h>
#include <stdio.h>

/* AlphaRNG retrieved data */
struct rng_data {
	int device_count;
	unsigned char rng_status;
	char device_error_message[256];
	char device_id[16];
	char device_model[16];
	unsigned char major_version;
	unsigned char minor_version;
	int health_status;
	unsigned char random_buffer[16];
	unsigned char entropy_buffer[32];
	unsigned char test_data_buffer[16];
	uint16_t freq_table_1[256];
	uint16_t freq_table_2[256];
	char device_path[128];
};

/*
  *** MAIN ***
*/
int main() {

	struct rng_data rand_data;
	int call_ret_value;
	int ret_val;
	int i;

	/* Create a connection context using RSA-2048, HMAC-SHA-160 and AES-256-GCM security attributes */
	struct alrng_context *ctxt = alrng_create_ctxt(rsa_2048_key, hmac_sha_160, aes_256_gcm, "");
	if (NULL == ctxt) {
		printf("Could not create context\n");
		return -1;
	}

	/* Connect to the first AlphaRNG device found */
	call_ret_value = alrng_connect(ctxt, 0);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve number of AlphaRNG devices connected */
	rand_data.device_count = alrng_get_device_count(ctxt);

	/* Retrieving device path */
	call_ret_value = alrng_retrieve_device_path(ctxt, rand_data.device_path, sizeof(rand_data.device_path), 0);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device rng status */
	call_ret_value = alrng_retrieve_rng_status(ctxt, &rand_data.rng_status);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device serial number */
	call_ret_value = alrng_retrieve_device_id(ctxt, rand_data.device_id, sizeof(rand_data.device_id));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device model */
	call_ret_value = alrng_retrieve_device_model(ctxt, rand_data.device_model, sizeof(rand_data.device_model));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device major version */
	call_ret_value = alrng_retrieve_device_major_version(ctxt, &rand_data.major_version);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device minor version */
	call_ret_value = alrng_retrieve_device_minor_version(ctxt, &rand_data.minor_version);
	if (call_ret_value) {
		goto error;
	}

	/* Run device diagnostics */
	rand_data.health_status = alrng_run_health_test(ctxt);

	/* Retrieve noise from source 1 */
	call_ret_value = alrng_get_noise_source_1(ctxt, rand_data.random_buffer, sizeof(rand_data.random_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve noise from source 2 */
	call_ret_value = alrng_get_noise_source_2(ctxt, rand_data.random_buffer, sizeof(rand_data.random_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve entropy using SHA-256 entropy extractor */
	call_ret_value = alrng_extract_sha256_entropy(ctxt, rand_data.random_buffer, sizeof(rand_data.random_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve entropy using SHA-512 entropy extractor */
	call_ret_value = alrng_extract_sha512_entropy(ctxt, rand_data.random_buffer, sizeof(rand_data.random_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve noise from device */
	call_ret_value = alrng_get_noise(ctxt, rand_data.random_buffer, sizeof(rand_data.random_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve test data */
	call_ret_value = alrng_get_test_data(ctxt, rand_data.test_data_buffer, sizeof(rand_data.test_data_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve entropy */
	call_ret_value = alrng_get_entropy(ctxt, rand_data.entropy_buffer, sizeof(rand_data.entropy_buffer));
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of entropy into a file */
	call_ret_value = alrng_entropy_to_file(ctxt, "entropy.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of entropy extracted using SHA-256 method into a file */
	call_ret_value = alrng_extract_sha256_entropy_to_file(ctxt, "entropy-sha256-extracted.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of entropy extracted using SHA-512 method into a file */
	call_ret_value = alrng_extract_sha512_entropy_to_file(ctxt, "entropy-sha512-extracted.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of noise bytes from source 1 into a file */
	call_ret_value = alrng_noise_source_one_to_file(ctxt, "noise-source-1.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of noise bytes from source 2 into a file */
	call_ret_value = alrng_noise_source_two_to_file(ctxt, "noise-source-2.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve 16 bytes of noise bytes into a file */
	call_ret_value = alrng_noise_to_file(ctxt, "noise.bin", 16);
	if (call_ret_value) {
		goto error;
	}

	/* Retrieve device frequency tables from noise sources */
	call_ret_value = alrng_retrieve_frequency_tables(ctxt, rand_data.freq_table_1, rand_data.freq_table_1);
	if (call_ret_value) {
		goto error;
	}


	/* Print retrieved data */
	printf("========================================\n");
	printf("             sample_c.c\n");
	printf("========================================\n");
	printf("device path: %s\n", rand_data.device_path);
	printf("device count: %d\n", rand_data.device_count);
	printf("device rng status: %d\n", (int)rand_data.rng_status);
	printf("device serial number: %s\n", rand_data.device_id);
	printf("device model: %s\n", rand_data.device_model);
	printf("device major version: %d\n", (int)rand_data.major_version);
	printf("device minor version: %d\n", (int)rand_data.minor_version);
	printf("device health status (0 - good): %d\n", rand_data.health_status);
	printf("device test data: ");

	for (i = 0; i < (int)sizeof(rand_data.test_data_buffer); i++) {
		printf("%d ", rand_data.test_data_buffer[i]);
	}
	printf("\n");
	printf("sample entropy bytes: ");

	for (i = 0; i < (int)sizeof(rand_data.entropy_buffer); i++) {
		printf("%d ", rand_data.entropy_buffer[i]);
	}
	printf("\n");
	goto close_and_exit;

error:
	ret_val = call_ret_value;
	if (ret_val == -1) {
		printf("Function invoked with an invalid argument\n");
	} else {
		ret_val = alrng_get_last_error(ctxt, rand_data.device_error_message, sizeof(rand_data.device_error_message));
		if (!ret_val) {
			printf("%s\n", rand_data.device_error_message);
		}
	}


close_and_exit:
	/* Close any active connection and destroy allocated context */
	ret_val = alrng_destroy_ctxt(ctxt);
	if (ret_val) {
		printf("Could not destroy the context\n");
		return ret_val;
	}
	return 0;
}
