(function handler(headers, url, method) {
  return {
    code: 200,
    body: method + ' ' + url + ' ' + headers['User-Agent'],
  };
})
