import test from 'node:test';
import assert from 'node:assert/strict';
import {SnapshotOrder} from '../web/modules/network/network-sync.js';
const snap=(epoch,sequence,now=1)=>({player:{id:'p'},now,_sync:{epoch,sequence}});
test('late poll cannot undo a newer command in the same simulation tick',()=>{
 const order=new SnapshotOrder();
 assert.equal(order.accept(snap('a',1)),true);
 assert.equal(order.accept(snap('a',3)),true);
 assert.equal(order.accept(snap('a',2)),false);
 assert.equal(order.accept(snap('a',3)),false);
});
test('server restart permits a new sequence without accepting delayed old-process replies',()=>{
 const order=new SnapshotOrder();
 assert.equal(order.accept(snap('a',99,20)),true);
 assert.equal(order.accept(snap('b',1,10)),true);
 assert.equal(order.accept(snap('a',100,21)),false);
 assert.equal(order.accept(snap('b',2,11)),true);
});
test('legacy servers retain time ordering and new characters reset it',()=>{
 const order=new SnapshotOrder();
 assert.equal(order.accept({player:{id:'p'},now:5}),true);
 assert.equal(order.accept({player:{id:'p'},now:4}),false);
 assert.equal(order.accept({player:{id:'q'},now:1}),true);
});
