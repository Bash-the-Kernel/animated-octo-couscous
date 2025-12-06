export class MarketDataClient {
  constructor(url) {
    this.url = url;
    this.ws = null;
    this.listeners = {
      trade: [],
      book_update: [],
      connected: [],
      disconnected: []
    };
  }

  connect() {
    this.ws = new WebSocket(this.url);
    
    this.ws.onopen = () => {
      console.log('WebSocket connected');
      this.listeners.connected.forEach(cb => cb());
    };
    
    this.ws.onclose = () => {
      console.log('WebSocket disconnected');
      this.listeners.disconnected.forEach(cb => cb());
      
      // Reconnect after 5 seconds
      setTimeout(() => this.connect(), 5000);
    };
    
    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
    
    this.ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        
        if (data.type === 'trade') {
          this.listeners.trade.forEach(cb => cb(data));
        } else if (data.type === 'book_update') {
          this.listeners.book_update.forEach(cb => cb(data));
        }
      } catch (e) {
        console.error('Failed to parse message:', e);
      }
    };
  }

  on(event, callback) {
    if (this.listeners[event]) {
      this.listeners[event].push(callback);
    }
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }
}
