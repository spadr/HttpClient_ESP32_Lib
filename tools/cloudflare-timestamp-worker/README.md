# HttpClient_ESP32_Lib Timestamp Worker

A simple Cloudflare Worker that returns the current Unix timestamp.
This is used by the `HttpClient::syncTime()` helper in the ESP32 library to synchronize time via HTTPS, which is often more reliable than NTP in some network environments.

## Deployment

1.  Ensure you have `wrangler` installed.
2.  Deploy to Cloudflare:

```bash
npx wrangler deploy
```

## API

### `GET /`

Returns the current Unix timestamp (seconds since epoch) as plain text.

**Response:**
```text
1719823456
```

