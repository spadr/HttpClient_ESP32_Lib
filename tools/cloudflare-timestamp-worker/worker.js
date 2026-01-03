/**
 * HttpClient_ESP32_Lib Timestamp Worker
 * 
 * Simple Cloudflare Worker to return current Unix timestamp.
 * Used for ESP32 time synchronization as an alternative to NTP.
 * 
 * Endpoint: https://timestamp.canaspad.net/
 */

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // Only allow GET requests
    if (request.method !== 'GET') {
        return new Response('Method Not Allowed', { 
            status: 405,
            headers: { 'Allow': 'GET' } 
        });
    }

    // Check User-Agent
    const userAgent = request.headers.get('User-Agent');
    if (!userAgent || !userAgent.includes('HttpClient-ESP32-Lib')) {
        return new Response('Forbidden', { status: 403 });
    }

    // Return current Unix timestamp in seconds
    const now = Math.floor(Date.now() / 1000); 
    
    return new Response(now.toString(), {
      status: 200,
      headers: { 
          'Content-Type': 'text/plain',
          'Cache-Control': 'no-store, no-cache, must-revalidate, proxy-revalidate',
          'Access-Control-Allow-Origin': '*' // Allow CORS if needed
      }
    });
  }
};

