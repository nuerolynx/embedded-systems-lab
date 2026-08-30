# Security policy

## Reporting a vulnerability

Please do not publish credentials, access identifiers, exploit details, or sensitive deployment information in a public issue.

Report security concerns privately through the contact channel at [Nuerolynx](https://www.nuerolynx.com). Include the affected project, impact, reproduction conditions, and a safe way to follow up. Do not include working secrets.

## Secrets and access data

- Keep Wi-Fi credentials in the ignored `arduino_secrets.h` files.
- Keep card UIDs and deployment authorization values in the ignored `access_config.h` files.
- Treat any credential ever committed to a repository as compromised, even if the repository was later made private or the file was deleted.
- Rotate compromised values at their source. Rewriting Git history does not invalidate a credential.

## Supported status

The examples are prototypes and demonstrations. They are not hardened or certified security products. Security fixes will be evaluated for the current default branch; no formal support window is promised.
