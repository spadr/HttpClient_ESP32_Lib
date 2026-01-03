# HttpClient_ESP32_Lib Demo Worker

A publicly accessible Cloudflare Worker for library demonstration.
It provides simple echo endpoints without authentication requirements, used by the `src/esp32/main.cpp` example.

## Endpoints

- `GET /`: Simple welcome message
- `POST /post`: Echoes back the posted data (JSON supported)

## Deployment

Deploy to `demo1.canaspad.net/post`:

```bash
npx wrangler deploy
```

