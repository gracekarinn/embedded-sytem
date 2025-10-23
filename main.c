#include <asf.h>
#include <stdio.h>
#include <ioport.h>
#include <board.h>
#include "DHT.h"

int main(void)
{
	char printbuff[20];
	float temp;
	float hum;
	float last_suhu = 0.0;
	float last_hum = 0.0;
	uint8_t fail_count = 0;
	uint8_t success_count = 0;

	// Inisialisasi sistem
	board_init();
	
	// CRITICAL: Konfigurasi clock ke 32MHz secara eksplisit
	OSC.CTRL |= OSC_RC32MEN_bm;
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
	
	sysclk_init();
	pmic_init();
	
	gfx_mono_init();
	delay_ms(100);
	gpio_set_pin_high(LCD_BACKLIGHT_ENABLE_PIN);
	delay_ms(50);
	
	// Tampilkan pesan startup
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
	gfx_mono_draw_string("Selamat Datang!", 20, 12, &sysfont);
	delay_ms(1500);
	
	// Inisialisasi DHT22
	DHT22_init();
	
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
	gfx_mono_draw_string("Kelompok Tiga", 25, 12, &sysfont);
	delay_ms(1500);
	
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
	gfx_mono_draw_string("Sensor Suhu Kelembapan", 0, 12, &sysfont);
	delay_ms(1500);

	// Loop utama
	while (1)
	{
		// Baca DHT22
		uint8_t status = DHT22_read(&temp, &hum);
		
		// Kalibrasi kelembaban saja
		hum = hum / 40;

		if (status) // Berhasil
		{
			success_count++;
			fail_count = 0;
			last_suhu = temp;
			last_hum = hum;

			// Tampilkan data di LCD
			gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
			
			if (temp > 32) {
				sprintf(printbuff, "Panas ya?");
			} else if (temp > 18) {
				sprintf(printbuff, "Suhunya nyaman!");
			} else {
				sprintf(printbuff, "Duh?dingin oi!");
			}
			
			gfx_mono_draw_string(printbuff, 0, 0, &sysfont);
			
			// Temperature
			gfx_mono_draw_string("Temp:", 0, 12, &sysfont);
			sprintf(printbuff, "%d C", (int)temp);
			gfx_mono_draw_string(printbuff, 40, 12, &sysfont);
			
			// Humidity
			gfx_mono_draw_string("Hum:", 0, 24, &sysfont);
			sprintf(printbuff, "%d %%", (int)hum);
			gfx_mono_draw_string(printbuff, 40, 24, &sysfont);
			
			// Refresh cepat tanpa delay
		}
		else // Gagal
		{
			fail_count++;
			
			// Tampilkan error di LCD
			gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
			gfx_mono_draw_string("FAILED!", 0, 0, &sysfont);
			
			// Tampilkan pesan error
			gfx_mono_draw_string(DHT22_get_error_message(dht_last_error), 0, 12, &sysfont);
			
			// Tampilkan fail count
			sprintf(printbuff, "Fail:%d", fail_count);
			gfx_mono_draw_string(printbuff, 0, 24, &sysfont);
			
			// Tahan tampilan error 2 detik agar bisa dibaca
			delay_ms(2000);
			
			// Jika terlalu banyak gagal, coba reset DHT22
			if (fail_count >= 5) {
				gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
				gfx_mono_draw_string("Resetting...", 0, 8, &sysfont);
				DHT22_init();
				delay_ms(2000);
				fail_count = 0;
			}
			
			// Tampilkan last data jika ada
			if (success_count > 0) {
				gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
				gfx_mono_draw_string("LAST DATA:", 0, 0, &sysfont);
				sprintf(printbuff, "T:%.1fC", last_suhu);
				gfx_mono_draw_string(printbuff, 0, 12, &sysfont);
				sprintf(printbuff, "H:%.1f%%", last_hum);
				gfx_mono_draw_string(printbuff, 0, 24, &sysfont);
				delay_ms(2000);
			}
		}
		
		// Small delay untuk stabilitas sensor (DHT22 butuh minimal 2 detik antar pembacaan)
		delay_ms(2000);
	}
}