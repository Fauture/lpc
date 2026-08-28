// Verify isxdigit works (was missing -> account_login version check failed).
string version = "20544DE38C2160D6EAC9BFE07486242E";
int bad = 0;
int i;
printf("len=%d lenOdd=%d\n", strlen(version), strlen(version) & 1);
for (i = 0; i < strlen(version); i++) {
    if (!isxdigit(version[i])) { bad = 1; printf("non-hex at %d: %d\n", i, version[i]); }
}
printf("allHex=%s\n", bad ? "NO" : "YES");
printf("isdigit(53)=%d isalpha(65)=%d isxdigit(70)=%d\n", isdigit(53), isalpha(65), isxdigit(70));
