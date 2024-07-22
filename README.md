# Bloom Filter Project

## Compilation and Running

1. Ensure you have OpenSSL installed.
2. Place `rockyou.ISO-8859-1.txt` and `dictionary.txt` in the same directory as `bloom_filter.c`.
3. Compile the code:
   ```bash
   gcc -o bloom_filter bloom_filter.c -lssl -lcrypto
   ```
   

## Notes

This program takes an incredibly long time to run, but I verified it's accuracy with smaller chunks of the `rockyou.ISO-8859-1.txt`
To complete a full run, it will likely need to be submitted as a batch process for one of the DGX systems on campus.
