# Security Policy

## Supported Versions

| Version | Supported |
| ------- | --------- |
| 1.0.x   | Yes       |

## Reporting a Vulnerability

We take security vulnerabilities seriously. If you discover a security issue, please report it responsibly.

### How to Report

**Do NOT open a public GitHub issue for security vulnerabilities.**

Instead, please email security concerns to: **contact@micr.dev**

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Any suggested fixes (optional)

### What to Expect

- **Acknowledgment**: Within 48 hours
- **Initial Assessment**: Within 7 days
- **Resolution Timeline**: Depends on severity, typically 30-90 days

### Disclosure Policy

- We will work with you to understand and resolve the issue
- We will credit you in the release notes (unless you prefer anonymity)
- Please allow reasonable time for a fix before public disclosure

## Security Considerations

### Encryption

Undying Terminal supports optional XSalsa20 encryption via libsodium:

- **Algorithm**: XSalsa20 (256-bit key, 24-byte nonce)
- **Note**: Encryption provides confidentiality but not authentication (no MAC/AEAD)

For sensitive environments, we recommend:
- Enable encryption (`shared_key_hex` in config)
- Use strong random keys (32 bytes of random hex)
- Combine with VPN for defense-in-depth
- Restrict firewall to known IPs

### Authentication

- Sessions are authenticated via client ID + passkey
- Passkeys are transmitted in plaintext during initial handshake (use encryption!)
- Keep passkeys secure and rotate periodically

### Network Exposure

If exposing to the internet:
- Always enable encryption
- Use non-standard ports
- Implement firewall rules restricting source IPs
- Monitor for unauthorized connection attempts
- Consider binding to localhost and using VPN

### Best Practices

1. **Generate strong passkeys**:
   ```powershell
   $bytes = New-Object byte[] 32
   [Security.Cryptography.RNGCryptoServiceProvider]::Create().GetBytes($bytes)
   -join ($bytes | ForEach-Object { $_.ToString("x2") })
   ```

2. **Rotate encryption keys periodically**

3. **Monitor server logs** for suspicious activity

4. **Keep Undying Terminal updated** to receive security fixes
