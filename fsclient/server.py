import http.server
import socketserver
import logging
from io import BytesIO

class CustomHandler(http.server.BaseHTTPRequestHandler):
    def _log_request_headers(self):
        # Log the request method, path, and headers
        logging.info(f"Request: {self.command} {self.path}")
        logging.info("Headers:\n" + str(self.headers))

    def _log_post_data(self):
        # Read and log the POST data
        content_length = int(self.headers.get('Content-Length', 0))
        if content_length > 0:
            post_data = self.rfile.read(content_length)  # Read the POST data
            logging.info(f"POST data:\n{post_data.decode('utf-8')}")
            
            

    def do_POST(self):
        self._log_request_headers()
        self._log_post_data()

        # Send a response back to the client
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(b'POST request received')
        logging.info("\n\n*********************************************************************************\n\n")

    def do_GET(self):
        self._log_request_headers()

        # Send a simple response for GET requests
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(b'GET request received')
        logging.info("\n\n*********************************************************************************\n\n")

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

    PORT = 8000
    with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
        logging.info(f"Serving on port {PORT}")
        httpd.serve_forever()
