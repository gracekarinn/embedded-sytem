#ifndef DHT_H_
#define DHT_H_

#include <asf.h>

// DHT22 menggunakan PORTC PIN2 (J1 pin 3 - RXD)
// Pin ini tidak konflik dengan LCD atau fungsi onboard lainnya
#define DHT_DDR PORTC.DIR
#define DHT_PORT PORTC.OUT
#define DHT_PIN PORTC.IN
#define DHT_PIN_BM PIN2_bm

// Error codes
#define DHT_OK 0
#define DHT_ERROR_TIMEOUT_START 1    // Sensor tidak merespons
#define DHT_ERROR_TIMEOUT_ACK 2      // Tidak ada ACK signal
#define DHT_ERROR_TIMEOUT_DATA 3     // Tidak ada data start
#define DHT_ERROR_TIMEOUT_BIT 4      // Timeout saat baca bit
#define DHT_ERROR_CHECKSUM 5         // Checksum tidak cocok

uint8_t dht_last_error = 0; // Menyimpan kode error terakhir

void DHT22_init(void)
{
	DHT_DDR |= DHT_PIN_BM;    // Set PIN sebagai output
	DHT_PORT |= DHT_PIN_BM;   // Set high
	delay_ms(100);            // Beri waktu DHT22 untuk stabilisasi
	dht_last_error = DHT_OK;
}

uint8_t DHT22_read(float *dht_temperatura, float *dht_humedad)
{
	uint8_t bits[5];
	uint8_t i, j = 0;
	uint16_t contador;

	// Step 1: Kirim start signal
	DHT_DDR |= DHT_PIN_BM;     // Set sebagai output
	DHT_PORT &= ~DHT_PIN_BM;   // Set low
	delay_ms(5);               // Tunggu lebih lama untuk stabilitas (was 2ms)
	DHT_PORT |= DHT_PIN_BM;    // Set high
	delay_us(30);              // Tunggu 20-40us
	DHT_DDR &= ~DHT_PIN_BM;    // Set sebagai input
	delay_us(10);              // Beri waktu pin untuk stabilisasi
	
	// Step 2: Tunggu DHT22 merespons (pull low)
	contador = 0;
	while (DHT_PIN & DHT_PIN_BM)
	{
		delay_us(2);
		contador++;
		if (contador > 60) // Timeout 120us (was 100us)
		{
			DHT_DDR |= DHT_PIN_BM;
			DHT_PORT |= DHT_PIN_BM;
			dht_last_error = DHT_ERROR_TIMEOUT_START;
			return 0;
		}
	}
	
	// Step 3: Tunggu DHT22 pull high (acknowledge)
	contador = 0;
	while (!(DHT_PIN & DHT_PIN_BM))
	{
		delay_us(2);
		contador++;
		if (contador > 50) // Timeout 100us
		{
			DHT_DDR |= DHT_PIN_BM;
			DHT_PORT |= DHT_PIN_BM;
			dht_last_error = DHT_ERROR_TIMEOUT_ACK;
			return 0;
		}
	}
	
	// Step 4: Tunggu DHT22 pull low sebelum data
	contador = 0;
	while (DHT_PIN & DHT_PIN_BM)
	{
		delay_us(2);
		contador++;
		if (contador > 50) // Timeout 100us
		{
			DHT_DDR |= DHT_PIN_BM;
			DHT_PORT |= DHT_PIN_BM;
			dht_last_error = DHT_ERROR_TIMEOUT_DATA;
			return 0;
		}
	}

	// Step 5: Baca 40 bits data (5 bytes)
	for (j = 0; j < 5; j++)
	{
		uint8_t result = 0;
		for (i = 0; i < 8; i++)
		{
			// Tunggu pin jadi high
			contador = 0;
			while (!(DHT_PIN & DHT_PIN_BM))
			{
				delay_us(2);
				contador++;
				if (contador > 60) // Timeout 120us (was 100us)
				{
					DHT_DDR |= DHT_PIN_BM;
					DHT_PORT |= DHT_PIN_BM;
					dht_last_error = DHT_ERROR_TIMEOUT_BIT;
					return 0;
				}
			}
			
			// Ukur durasi high untuk menentukan bit 0 atau 1
			// Bit 0 = ~26-28us high, Bit 1 = ~70us high
			delay_us(35);  // Tunggu di tengah antara 28 dan 70
			
			if (DHT_PIN & DHT_PIN_BM) {
				result |= (1<<(7-i));  // Jika masih high setelah 35us = bit 1
			}
			// Jika sudah low = bit 0
			
			// Tunggu pin jadi low
			contador = 0;
			while (DHT_PIN & DHT_PIN_BM)
			{
				delay_us(2);
				contador++;
				if (contador > 60) // Timeout 120us (was 100us)
				{
					DHT_DDR |= DHT_PIN_BM;
					DHT_PORT |= DHT_PIN_BM;
					dht_last_error = DHT_ERROR_TIMEOUT_BIT;
					return 0;
				}
			}
		}
		bits[j] = result;
	}

	// Kembalikan pin ke idle state
	DHT_DDR |= DHT_PIN_BM;
	DHT_PORT |= DHT_PIN_BM;
	
	// Step 6: Verifikasi checksum
	uint8_t checksum = (uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]);
	
	if (checksum == bits[4])
	{
		// Parse humidity (16-bit)
		uint16_t rawhumedad = (bits[0] << 8) | bits[1];
		// Parse temperature (16-bit)
		uint16_t rawtemperatura = (bits[2] << 8) | bits[3];
		
		// Cek bit sign untuk temperature
		if (rawtemperatura & 0x8000)
		{
			*dht_temperatura = (float)((rawtemperatura & 0x7FFF) / 10.0) * -1.0;
		}
		else
		{
			*dht_temperatura = (float)(rawtemperatura) / 10.0;
		}

		*dht_humedad = (float)(rawhumedad) / 10.0;
		
		dht_last_error = DHT_OK;
		return 1; // Pembacaan berhasil
	}
	
	dht_last_error = DHT_ERROR_CHECKSUM;
	return 0; // Checksum gagal
}

// Fungsi untuk mendapatkan pesan error
const char* DHT22_get_error_message(uint8_t error_code)
{
	switch(error_code)
	{
		case DHT_OK:
		return "OK";
		case DHT_ERROR_TIMEOUT_START:
		return "ERR1:No Response";
		case DHT_ERROR_TIMEOUT_ACK:
		return "ERR2:No ACK";
		case DHT_ERROR_TIMEOUT_DATA:
		return "ERR3:No Data";
		case DHT_ERROR_TIMEOUT_BIT:
		return "ERR4:Bit Timeout";
		case DHT_ERROR_CHECKSUM:
		return "ERR5:Checksum";
		default:
		return "ERR:Unknown";
	}
}

#endif /* DHT_H_ */