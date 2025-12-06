import React from 'react';

export function TradesTape({ trades }) {
  return (
    <div style={{ fontFamily: 'monospace' }}>
      <h3>Recent Trades</h3>
      <table style={{ width: '100%', borderCollapse: 'collapse' }}>
        <thead>
          <tr style={{ borderBottom: '1px solid #333' }}>
            <th style={{ textAlign: 'left', padding: '5px' }}>Time</th>
            <th style={{ textAlign: 'right', padding: '5px' }}>Price</th>
            <th style={{ textAlign: 'right', padding: '5px' }}>Quantity</th>
            <th style={{ textAlign: 'center', padding: '5px' }}>Side</th>
          </tr>
        </thead>
        <tbody>
          {trades.slice(0, 20).map((trade, i) => {
            const time = new Date(trade.timestamp / 1000000).toLocaleTimeString();
            const side = trade.buyer_is_aggressor ? 'BUY' : 'SELL';
            const color = trade.buyer_is_aggressor ? '#00ff00' : '#ff0000';
            
            return (
              <tr key={i}>
                <td style={{ padding: '5px', fontSize: '0.9em', color: '#888' }}>
                  {time}
                </td>
                <td style={{ textAlign: 'right', padding: '5px', fontWeight: 'bold' }}>
                  {trade.price.toFixed(2)}
                </td>
                <td style={{ textAlign: 'right', padding: '5px' }}>
                  {trade.quantity}
                </td>
                <td style={{ textAlign: 'center', padding: '5px', color }}>
                  {side}
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
