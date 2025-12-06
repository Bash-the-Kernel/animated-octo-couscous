import React from 'react';

export function OrderBook({ bids, asks }) {
  return (
    <div style={{ display: 'flex', gap: '20px', fontFamily: 'monospace' }}>
      <div style={{ flex: 1 }}>
        <h3 style={{ color: '#00ff00' }}>Bids</h3>
        <table style={{ width: '100%', borderCollapse: 'collapse' }}>
          <thead>
            <tr style={{ borderBottom: '1px solid #333' }}>
              <th style={{ textAlign: 'right', padding: '5px' }}>Price</th>
              <th style={{ textAlign: 'right', padding: '5px' }}>Quantity</th>
            </tr>
          </thead>
          <tbody>
            {bids.slice(0, 10).map((bid, i) => (
              <tr key={i} style={{ color: '#00ff00' }}>
                <td style={{ textAlign: 'right', padding: '5px' }}>
                  {bid.price.toFixed(2)}
                </td>
                <td style={{ textAlign: 'right', padding: '5px' }}>
                  {bid.quantity}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      
      <div style={{ flex: 1 }}>
        <h3 style={{ color: '#ff0000' }}>Asks</h3>
        <table style={{ width: '100%', borderCollapse: 'collapse' }}>
          <thead>
            <tr style={{ borderBottom: '1px solid #333' }}>
              <th style={{ textAlign: 'right', padding: '5px' }}>Price</th>
              <th style={{ textAlign: 'right', padding: '5px' }}>Quantity</th>
            </tr>
          </thead>
          <tbody>
            {asks.slice(0, 10).map((ask, i) => (
              <tr key={i} style={{ color: '#ff0000' }}>
                <td style={{ textAlign: 'right', padding: '5px' }}>
                  {ask.price.toFixed(2)}
                </td>
                <td style={{ textAlign: 'right', padding: '5px' }}>
                  {ask.quantity}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
