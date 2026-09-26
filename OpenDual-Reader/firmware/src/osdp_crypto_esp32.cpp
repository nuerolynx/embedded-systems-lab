// OpenDual port. Replaces upstream PlatformIO TinyAES libc-rand() backend.
#include <Arduino.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <mbedtls/aes.h>
#include <stdlib.h>
extern "C" {
int64_t osdp_millis_now(){return esp_timer_get_time()/1000;}
void osdp_crypt_setup(){}
void osdp_crypt_teardown(){}
void osdp_fill_random(uint8_t *buf,int len){if(len>0)esp_fill_random(buf,len);}
void osdp_encrypt(uint8_t *key,uint8_t *iv,uint8_t *data,int len){
  mbedtls_aes_context ctx; mbedtls_aes_init(&ctx);
  int rc=mbedtls_aes_setkey_enc(&ctx,key,128);
  if(!rc) rc=iv ? mbedtls_aes_crypt_cbc(&ctx,MBEDTLS_AES_ENCRYPT,len,iv,data,data)
                 : mbedtls_aes_crypt_ecb(&ctx,MBEDTLS_AES_ENCRYPT,data,data);
  mbedtls_aes_free(&ctx); if(rc)abort();
}
void osdp_decrypt(uint8_t *key,uint8_t *iv,uint8_t *data,int len){
  mbedtls_aes_context ctx; mbedtls_aes_init(&ctx);
  int rc=mbedtls_aes_setkey_dec(&ctx,key,128);
  if(!rc)rc=iv ? mbedtls_aes_crypt_cbc(&ctx,MBEDTLS_AES_DECRYPT,len,iv,data,data)
                : mbedtls_aes_crypt_ecb(&ctx,MBEDTLS_AES_DECRYPT,data,data);
  mbedtls_aes_free(&ctx); if(rc)abort();
}
}
