(function handler(headers, url, method) {
  if (method !== 'GET') {
    return { code: 405, body: 'Invalid method' };
  }

  if (url === '/') {
    return { code: 200, body: 'Main page' };
  }

  if (url === '/about') {
    return { code: 200, body: 'About this project' };
  }

  return {
    code: 404,
    body: 'Not found',
  };
})
