#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libhackrf/hackrf.h>
#include <unistd.h>
#include "RF_modulation.h"

static unsigned int txvga_gain = 47;
static const long long sample_rate_hz = 2000000LL;
static const long long freq_hz = 433850000LL;

uint8_t *iq_samples = NULL;
int iq_samples_length = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int init_hackrf(void);
void shutdown_hackrf(void);
int configure_hackrf(unsigned long long sample_rate, unsigned long long frequency, unsigned int gain);

int tx_callback(hackrf_transfer* transfer);

static hackrf_device *device = NULL;

int init_hackrf(void)
{
    int result = hackrf_init();
    if( result != HACKRF_SUCCESS )
    {
        return result;
    }

    hackrf_device_list_t *list = hackrf_device_list();

    if (list->devicecount < 1 )
    {
		printf("No HackRF boards found.\n");
		return -1;
    }

    result = hackrf_device_list_open(list, 0, &device); //open first device
    if (result != HACKRF_SUCCESS)
    {
        return result;
    }
    printf("hackrf serial: %s\n", list->serial_numbers[0]);
    printf("hackrf USB device: %i\n", list->usb_device_index[0]);

    hackrf_device_list_free(list);

    result = configure_hackrf(sample_rate_hz, freq_hz, txvga_gain);

    printf("hackrf sample rate: %lluMHz\n", sample_rate_hz / 1000000);
    printf("hackrf TX gain: %idB\n", txvga_gain);
    printf("hackrf TX frequency: %fMHz\n", (float)freq_hz/1000000);


    return result;
}

void shutdown_hackrf(void)
{
    int result = hackrf_stop_tx(device);
    if (result != HACKRF_SUCCESS)
    {
    }

    result = hackrf_close(device);
    if (result != HACKRF_SUCCESS)
    {
    }

    printf("hackrf device shutdown\n");

    hackrf_exit();
}

int configure_hackrf(unsigned long long sample_rate, unsigned long long frequency, unsigned int gain)
{
    int result = hackrf_set_sample_rate(device, sample_rate);
    if( result != HACKRF_SUCCESS )
    {
        return result;
    }

    result = hackrf_set_txvga_gain(device, gain);
    if( result != HACKRF_SUCCESS )
    {
        return result;
    }

    result = hackrf_set_freq(device, frequency);
	if( result != HACKRF_SUCCESS )
    {
        return result;
    }

    return 0;

}


int main(int argc, const char** argv)
{
    if (argc == 2)
    {
        txvga_gain = strtoul(argv[1], NULL, 10);
    }

    //Control the 433.92MHz doorbell
    const long signal_duration_us = (SYMBOL_PERIOD * SYMBOLS_PER_FRAME + BREAK_BETWEEN);

    printf("Signal duration(us)=%ld\n", signal_duration_us);

    const long total_samples = signal_duration_us * (sample_rate_hz / 1000000l);

    const long samples_per_symbol = SYMBOL_PERIOD * (sample_rate_hz / 1000000l);

    const long samples_per_long_pulse = BIG_PULSE * (sample_rate_hz / 1000000l);
    const long samples_per_short_pulse = SMALL_PULSE * (sample_rate_hz / 1000000l);

    const uint8_t single_iq = 100;

    iq_samples_length = total_samples *2;

    iq_samples = malloc(iq_samples_length);

    printf("Total samples generated=%ld\n", total_samples);
    printf("Total broadcast duration (ms)=%ld\n", 100 * signal_duration_us / 1000);

    memset(iq_samples, 0x00, iq_samples_length);

    uint16_t *ptr = (uint16_t*)&iq_samples[0];

    printf("Frame bits:");
    for (long i = 0; i < SYMBOLS_PER_FRAME; i++)
    {
        const int symbol = (frame >> i) & 0x01;
        const long samples_to_fill = symbol ? samples_per_long_pulse : samples_per_short_pulse;
        printf("%i", symbol);

        memset((uint8_t*)ptr, single_iq, samples_to_fill *2);
        ptr += samples_per_symbol;
    }
    printf("\n");

    int result = init_hackrf();
    if (result != 0)
    {
        printf("Hackrf error: %s\n", hackrf_error_name(result));
        return 1;
    }

    pthread_mutex_lock(&mutex);
    hackrf_start_tx(device, &tx_callback, NULL);
    sleep(2);


    //It looks weird with the other lock, but when the tx is done it unlocks and the application can exit
    pthread_mutex_lock(&mutex);

    shutdown_hackrf();
    free(iq_samples);

    return 0;
}


int tx_callback(hackrf_transfer* transfer)
{
    const int max_bytes = transfer->valid_length;

    static int iteration = 0;

    uint8_t *ptr = transfer->buffer;


    int bytes_written = 0;
    static int offset = 0;
    int bytes_to_write = 0;

    while (bytes_written < max_bytes)
    {
        if (offset == 0)
        {
            // beginning of a frame
            iteration++;
        }

        bytes_to_write = iq_samples_length - offset;
        int bytes_available = max_bytes - bytes_written;
        if (bytes_to_write > bytes_available)
        {
            bytes_to_write = bytes_available;
        }

        memcpy(ptr + bytes_written, iq_samples + offset, bytes_to_write);

        bytes_written += bytes_to_write;

        //printf("Bytes written=%d, total=%d\n", bytes_to_write, bytes_written);
        offset = 0;

        if (iteration >= REPEATS)
        {
            // All data has been sent
            pthread_mutex_unlock(&mutex);
            memset(ptr + bytes_written, 0x00, max_bytes - bytes_written);
            return 0;
        }
    }


    offset = bytes_to_write;

    return 0;
}
