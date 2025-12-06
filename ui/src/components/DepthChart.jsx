import React from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

export function DepthChart({ bids, asks }) {
  // Calculate cumulative depth
  const bidDepth = [];
  let cumBid = 0;
  for (const bid of bids) {
    cumBid += bid.quantity;
    bidDepth.push({ price: bid.price, depth: cumBid });
  }
  
  const askDepth = [];
  let cumAsk = 0;
  for (const ask of asks) {
    cumAsk += ask.quantity;
    askDepth.push({ price: ask.price, depth: cumAsk });
  }
  
  // Combine for chart
  const data = [
    ...bidDepth.reverse().map(d => ({ ...d, type: 'bid' })),
    ...askDepth.map(d => ({ ...d, type: 'ask' }))
  ];
  
  return (
    <div>
      <h3>Depth Chart</h3>
      <ResponsiveContainer width="100%" height={300}>
        <AreaChart data={data}>
          <CartesianGrid strokeDasharray="3 3" stroke="#333" />
          <XAxis 
            dataKey="price" 
            type="number"
            domain={['dataMin', 'dataMax']}
            stroke="#888"
          />
          <YAxis stroke="#888" />
          <Tooltip 
            contentStyle={{ backgroundColor: '#1a1a1a', border: '1px solid #333' }}
          />
          <Area 
            type="stepAfter" 
            dataKey="depth" 
            stroke="#00ff00" 
            fill="#00ff0033"
            isAnimationActive={false}
          />
        </AreaChart>
      </ResponsiveContainer>
    </div>
  );
}
