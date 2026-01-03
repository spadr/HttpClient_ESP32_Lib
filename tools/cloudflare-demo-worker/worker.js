/**
 * HttpClient_ESP32_Lib Demo Worker
 * 
 * Simple Cloudflare Worker for library demonstration purposes.
 * Publicly accessible endpoint (no auth required).
 * 
 * Endpoint: https://demo1.canaspad.net/
 */

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const headers = Object.fromEntries(request.headers);

    // Check User-Agent
    const userAgent = request.headers.get('User-Agent');
    if (!userAgent || !userAgent.includes('HttpClient-ESP32-Lib')) {
        return new Response('Forbidden', { status: 403 });
    }

    // Add CORS headers
    const corsHeaders = {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
      'Access-Control-Allow-Headers': '*',
    };

    if (request.method === 'OPTIONS') {
      return new Response(null, { headers: corsHeaders });
    }

    // 1. POST Echo (for demo)
    if (url.pathname === '/post' && request.method === 'POST') {
      let data = '';
      let json = null;
      const contentType = headers['content-type'] || '';

      try {
        if (contentType.includes('application/json')) {
          json = await request.json();
          data = JSON.stringify(json);
        } else {
          data = await request.text();
        }
      } catch (e) {
        data = '';
      }

      return new Response(JSON.stringify({
        message: 'Hello from demo1.canaspad.net/post!',
        received: {
          method: 'POST',
          headers: headers,
          json: json,
          data: data
        },
        timestamp: Date.now()
      }, null, 2), {
        headers: {
          'Content-Type': 'application/json',
          ...corsHeaders
        }
      });
    }

    // 2. GET Check
    if (url.pathname === '/get' || url.pathname === '/') {
       return new Response(JSON.stringify({
        message: 'Welcome to HttpClient_ESP32_Lib Demo!',
        ip: headers['cf-connecting-ip'],
        method: request.method
      }, null, 2), {
        headers: {
          'Content-Type': 'application/json',
          ...corsHeaders
        }
      });
    }

    return new Response('Not Found', { status: 404, headers: corsHeaders });
  }
};

