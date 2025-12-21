/**
 * HttpClient_ESP32_Lib E2E Test Worker
 * 
 * Cloudflare Workers script to simulate backend endpoints for ESP32 E2E testing.
 * Compatible with httpbin.org functionality used in tests.
 * 
 * SECURITY: Requires 'X-E2E-Token' header matching E2E_SECRET_TOKEN env var.
 */

export default {
  async fetch(request, env, ctx) {
    // --- Security Check ---
    const expectedToken = env.E2E_SECRET_TOKEN;
    const requestToken = request.headers.get('X-E2E-Token');

    // Only enforce token if environment variable is set
    if (expectedToken && requestToken !== expectedToken) {
      return new Response('Unauthorized: Invalid or missing X-E2E-Token', { 
        status: 401,
        headers: { 'Content-Type': 'text/plain' }
      });
    }

    const url = new URL(request.url);
    const headers = Object.fromEntries(request.headers);

    // --- Helper Functions ---
    const jsonResponse = (data, status = 200) => {
      return new Response(JSON.stringify(data, null, 2), {
        status,
        headers: {
          'Content-Type': 'application/json',
          'Server': 'canaspad-e2e-worker'
        }
      });
    };

    // --- Endpoints ---

    // 1. GET Request Check
    if (url.pathname === '/get') {
      return jsonResponse({
        args: Object.fromEntries(url.searchParams),
        headers: headers,
        origin: headers['cf-connecting-ip'] || 'unknown',
        url: request.url
      });
    }

    // 2. POST Request Check
    if (url.pathname === '/post') {
      const contentType = headers['content-type'] || '';
      let data = '';
      let json = null;

      if (request.method !== 'POST') {
        return new Response('Method Not Allowed', { status: 405 });
      }

      try {
        if (contentType.includes('application/json')) {
          json = await request.json();
          data = JSON.stringify(json);
        } else {
          data = await request.text();
        }
      } catch (e) {
        data = ''; // Body read error or empty
      }

      return jsonResponse({
        args: Object.fromEntries(url.searchParams),
        data: data,
        files: {},
        form: {},
        headers: headers,
        json: json,
        origin: headers['cf-connecting-ip'] || 'unknown',
        url: request.url
      });
    }

    // 3. Status Code Check
    // /status/418
    if (url.pathname.startsWith('/status/')) {
      const code = parseInt(url.pathname.split('/')[2]);
      if (isNaN(code)) return new Response('Invalid code', { status: 400 });
      return new Response(null, { status: code });
    }

    // 4. Delay Check (Timeout Test)
    // /delay/3 -> Wait 3 seconds
    if (url.pathname.startsWith('/delay/')) {
      const delaySec = Math.min(parseInt(url.pathname.split('/')[2]) || 0, 10); // Max 10s
      await new Promise(r => setTimeout(r, delaySec * 1000));
      return jsonResponse({
        args: Object.fromEntries(url.searchParams),
        headers: headers,
        origin: headers['cf-connecting-ip'] || 'unknown',
        url: request.url,
        delay: delaySec
      });
    }

    // 5. Bytes Response (Large Data Test)
    // /bytes/1024
    if (url.pathname.startsWith('/bytes/')) {
      const n = Math.min(parseInt(url.pathname.split('/')[2]) || 0, 100 * 1024); // Max 100KB
      const data = new Uint8Array(n);
      // Fill with random-ish data (fast generation)
      for(let i=0; i<n; i++) data[i] = i % 256;
      
      return new Response(data, {
        headers: {
          'Content-Type': 'application/octet-stream',
          'Content-Length': n.toString()
        }
      });
    }

    // 6. Redirect Check
    // /redirect/n
    if (url.pathname.startsWith('/redirect/')) {
        const parts = url.pathname.split('/');
        let n = parseInt(parts[2]) || 1;
        
        if (n <= 1) {
            return Response.redirect(`${url.origin}/get`, 302);
        } else {
            return Response.redirect(`${url.origin}/redirect/${n - 1}`, 302);
        }
    }

    // Default: 404
    return new Response('Not Found', { status: 404 });
  }
};
