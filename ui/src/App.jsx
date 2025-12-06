import React, { useState, useEffect } from 'react';
import { MarketDataClient } from './websocket';
import { OrderBook } from './components/OrderBook';
import { TradesTape } from './components/TradesTape';
import { DepthChart } from './components/DepthChart';

function App() {
  const [connected, setConnected] = useState(false);
  const [bids, setBids] = useState([]);
  const [asks, setAsks] = useState([]);
  const [trades, setTrades] = useState([]);
  const [stats, setStats] = useState({ orders: 0, trades: 0 });

  useEffect(() => {
    const client = new MarketDataClient('ws://localhost:9002');
    
    client.on('connected', () => setConnected(true));
    client.on('disconnected', () => setConnected(false));
    
    client.on('trade', (trade) => {
      setTrades(prev => [trade, ...prev].slice(0, 100));
      setStats(prev => ({ ...prev, trades: prev.trades + 1 }));
    });
    
    client.on('book_update', (update) => {
      if (update.side === 'buy') {
        setBids(prev => {
          const newBids = [...prev];
          const idx = newBids.findIndex(b => b.price === update.price);
          if (idx >= 0) {
            if (update.quantity === 0) {
              newBids.splice(idx, 1);
            } else {
              newBids[idx].quantity = update.quantity;
            }
          } else if (update.quantity > 0) {
            newBids.push({ price: update.price, quantity: update.quantity });
            newBids.sort((a, b) => b.price - a.price);
          }
          return newBids;
        });
      } else {
        setAsks(prev => {
          const newAsks = [...prev];
          const idx = newAsks.findIndex(a => a.price === update.price);
          if (idx >= 0) {
            if (update.quantity === 0) {
              newAsks.splice(idx, 1);
            } else {
              newAsks[idx].quantity = update.quantity;
            }
          } else if (update.quantity > 0) {
            newAsks.push({ price: update.price, quantity: update.quantity });
            newAsks.sort((a, b) => a.price - b.price);
          }
          return newAsks;
        });
      }
    });
    
    client.connect();
    
    // Fetch stats periodically
    const statsInterval = setInterval(async () => {
      try {
        const res = await fetch('http://localhost:8080/orders/count');
        const data = await res.json();
        setStats(prev => ({ ...prev, orders: data.processed }));
      } catch (e) {
        console.error('Failed to fetch stats:', e);
      }
    }, 1000);
    
    return () => {
      client.disconnect();
      clearInterval(statsInterval);
    };
  }, []);

  return (
    <div style={{ 
      backgroundColor: '#0a0a0a', 
      color: '#fff', 
      minHeight: '100vh',
      padding: '20px',
      fontFamily: 'Arial, sans-serif'
    }}>
      <header style={{ 
        borderBottom: '2px solid #333', 
        paddingBottom: '20px',
        marginBottom: '20px'
      }}>
        <h1 style={{ margin: 0 }}>HFT Exchange</h1>
        <div style={{ marginTop: '10px', fontSize: '0.9em', color: '#888' }}>
          <span style={{ 
            display: 'inline-block',
            width: '10px',
            height: '10px',
            borderRadius: '50%',
            backgroundColor: connected ? '#00ff00' : '#ff0000',
            marginRight: '5px'
          }} />
          {connected ? 'Connected' : 'Disconnected'}
          <span style={{ marginLeft: '20px' }}>
            Orders: {stats.orders.toLocaleString()}
          </span>
          <span style={{ marginLeft: '20px' }}>
            Trades: {stats.trades.toLocaleString()}
          </span>
        </div>
      </header>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '20px' }}>
        <div style={{ 
          backgroundColor: '#1a1a1a', 
          padding: '20px', 
          borderRadius: '8px',
          border: '1px solid #333'
        }}>
          <OrderBook bids={bids} asks={asks} />
        </div>

        <div style={{ 
          backgroundColor: '#1a1a1a', 
          padding: '20px', 
          borderRadius: '8px',
          border: '1px solid #333'
        }}>
          <TradesTape trades={trades} />
        </div>

        <div style={{ 
          backgroundColor: '#1a1a1a', 
          padding: '20px', 
          borderRadius: '8px',
          border: '1px solid #333',
          gridColumn: '1 / -1'
        }}>
          <DepthChart bids={bids} asks={asks} />
        </div>
      </div>
    </div>
  );
}

export default App;
