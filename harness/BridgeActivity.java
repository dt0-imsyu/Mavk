package org.mavk.probe;

import android.app.Activity;
import android.os.Bundle;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.widget.TextView;
import java.io.*;
import java.nio.*;
import java.nio.file.Files;
import java.util.concurrent.atomic.AtomicReference;
import org.json.*;

public final class BridgeActivity extends Activity {
    static {System.loadLibrary("mavk_probe");}
    private static native long createBridge(byte[] shader,boolean validation);
    private static native byte[] exchangeBridge(long owner,byte[] frame);
    private static native String finishBridge(long owner);
    private static final int MAGIC=0x314b564d,MAX=1024*1024;
    private long sequence;
    private int commands,negativeTests;
    private long wordsChecked;
    private int pixelsChecked;
    private static ByteBuffer bytes(int n){return ByteBuffer.allocate(n).order(ByteOrder.LITTLE_ENDIAN);}
    private static ByteBuffer wrap(byte[] b){return ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN);}
    private static void require(boolean value,String reason){if(!value)throw new IllegalStateException(reason);}
    private static byte[] readFrame(InputStream input)throws IOException {
        DataInputStream in=new DataInputStream(input);byte[] header=new byte[32];in.readFully(header);
        int payload=wrap(header).getInt(24);if(payload<0||payload>MAX)throw new IOException("Frame payload limit before allocation");
        byte[] result=new byte[32+payload];System.arraycopy(header,0,result,0,32);in.readFully(result,32,payload);return result;
    }
    private byte[] request(int op,byte[] payload){sequence++;ByteBuffer b=bytes(32+payload.length);b.putInt(MAGIC).putShort((short)1).putShort((short)32).putInt(op).putInt(0).putLong(sequence).putInt(payload.length).putInt(0).put(payload);return b.array();}
    private byte[] roundtrip(LocalSocket socket,byte[] request,int expected)throws IOException {
        // Deliberately split headers/payload. Server must not rely on read boundaries.
        OutputStream out=socket.getOutputStream();for(int offset=0;offset<request.length;){int n=Math.min(offset<32?3:4093,request.length-offset);out.write(request,offset,n);offset+=n;}
        byte[] response=readFrame(socket.getInputStream());ByteBuffer h=wrap(response),q=wrap(request);
        require(h.getInt(0)==MAGIC&&h.getShort(4)==1&&h.getShort(6)==32&&h.getInt(12)==1,"Response framing");
        require(h.getInt(8)==q.getInt(8)&&h.getLong(16)==q.getLong(16),"Response correlation");require(h.getInt(28)==expected,"Unexpected status "+h.getInt(28)+" expected "+expected);commands++;if(expected!=0)negativeTests++;
        byte[] payload=new byte[response.length-32];System.arraycopy(response,32,payload,0,payload.length);return payload;
    }
    private void test(LocalSocket socket)throws Exception {
        byte[] hello=roundtrip(socket,request(1,new byte[0]),0);require(hello.length==16&&wrap(hello).getInt(4)==MAX,"Capabilities");
        roundtrip(socket,request(999,new byte[0]),3);
        byte[] malformed=request(1,new byte[0]);malformed[4]=2;roundtrip(socket,malformed,1);
        roundtrip(socket,request(2,bytes(4).putInt(0).array()),5);
        for(int size:new int[]{16384,262144}){
            long id=wrap(roundtrip(socket,request(2,bytes(4).putInt(size).array()),0)).getLong();
            roundtrip(socket,request(5,bytes(16).putLong(id).putInt(0).putInt(size).array()),8);
            for(int round=0;round<8;round++){
                int seed=round*747796405+289133645;
                roundtrip(socket,request(4,bytes(12).putLong(id).putInt(seed).array()),0);
                byte[] result=roundtrip(socket,request(5,bytes(16).putLong(id).putInt(0).putInt(size).array()),0);require(result.length==size,"Read size");ByteBuffer data=wrap(result);
                for(int i=0;i<size/4;i++){require(data.getInt()==((i*1664525+1013904223)^seed),"GPU mismatch word "+i);wordsChecked++;}
            }
            roundtrip(socket,request(5,bytes(16).putLong(id).putInt(size-4).putInt(8).array()),2);
            roundtrip(socket,request(3,bytes(8).putLong(id).array()),0);
            roundtrip(socket,request(3,bytes(8).putLong(id).array()),4);
        }
        byte[] replay=request(1,new byte[0]);roundtrip(socket,replay,0);roundtrip(socket,replay,7);
        // Leave a resource alive; disconnect must retire it without a client destroy.
        roundtrip(socket,request(2,bytes(4).putInt(16384).array()),0);
    }
    private void vertexTest(LocalSocket socket)throws Exception {
        roundtrip(socket,request(1,new byte[0]),0);
        boolean matrix=getIntent().getBooleanExtra("matrix",false);
        byte[] vs,fs;
        try(InputStream input=getAssets().open(matrix?"matrix.vert.spv":"vertex-buffer.vert.spv")){vs=input.readAllBytes();}
        try(InputStream input=getAssets().open("offscreen.frag.spv")){fs=input.readAllBytes();}
        boolean indexed=matrix||getIntent().getBooleanExtra("indexed",false);
        int vertexCount=indexed?4:3,geometryBytes=16+12+vertexCount*16;
        ByteBuffer b=bytes((matrix?104:48)+vs.length+fs.length+geometryBytes+(indexed?28:0));
        b.putInt(64).putInt(64).putInt(vs.length).putInt(fs.length);
        if(matrix){
            b.putInt(1).putInt(64); // vertex-only push range
            for(int i=0;i<4;i++)b.putFloat(i==3?1:0);
            float[] transform={.5f,0,0,0,0,.5f,0,0,0,0,1,0,.25f,-.125f,0,1};
            for(float value:transform)b.putFloat(value);
        }else{for(int i=0;i<4;i++)b.putFloat(0);for(int i=0;i<4;i++)b.putFloat(i==3?1:0);}
        b.put(vs).put(fs);
        if(indexed)b.putInt(geometryBytes).putInt(1).putInt(3).putInt(12); // uint32 indices
        b.putInt(16).putInt(1).putInt(vertexCount).putInt(vertexCount*16);
        b.putInt(0).putInt(103).putInt(4); // location0, VK_FORMAT_R32G32_SFLOAT, offset4
        float[] values=indexed?new float[]{99,9,9,99,99,.75f,-.75f,99,99,0,.75f,99,99,-.75f,-.75f,99}:new float[]{99,-.75f,-.75f,99,99,.75f,-.75f,99,99,0,.75f,99};
        for(float value:values)b.putFloat(value);
        if(indexed)b.putInt(3).putInt(1).putInt(2);
        if(matrix){
            for(int size:new int[]{0,3,68,Integer.MAX_VALUE}){byte[] invalid=b.array().clone();wrap(invalid).putInt(20,size);roundtrip(socket,request(12,invalid),2);}
            byte[] invalid=b.array().clone();wrap(invalid).putInt(16,32);roundtrip(socket,request(12,invalid),2);
        }
        byte[] pixels=roundtrip(socket,request(matrix?12:indexed?11:10,b.array()),0);require(pixels.length==64*64*4,"Vertex readback size");
        for(int y=0;y<64;y++)for(int x=0;x<64;x++){
            float nx=2*(x+.5f)/64-1,ny=2*(y+.5f)/64-1;
            if(matrix){nx=2*(nx-.25f);ny=2*(ny+.125f);}
            float side=.375f-.5f*ny-Math.abs(nx);
            boolean inside=ny>-.67f&&ny<.67f&&side>.08f,outside=ny<-.83f||ny>.83f||side<-.08f;
            if(!inside&&!outside)continue;
            int[] expected={0,inside?64:0,inside?255:0,255};
            for(int k=0;k<4;k++)require(Math.abs((pixels[(y*64+x)*4+k]&255)-expected[k])<=1,"Vertex GPU pixel mismatch");
            pixelsChecked++;
        }
        require(pixelsChecked==(matrix?3906:3330),"Vertex pixel sample count: "+pixelsChecked);
    }
    private void sceneTest(LocalSocket socket)throws Exception {
        boolean retain=getIntent().getBooleanExtra("snapshot",false);int dimension=retain?1024:64,op=retain?14:13;
        roundtrip(socket,request(1,new byte[0]),0);byte[] vs,fs;
        try(InputStream input=getAssets().open("scene.vert.spv")){vs=input.readAllBytes();}
        try(InputStream input=getAssets().open("scene.frag.spv")){fs=input.readAllBytes();}
        int geometryBytes=16+24+9*24,indexedBytes=16+geometryBytes+9*4;
        ByteBuffer b=bytes(64+vs.length+fs.length+indexedBytes+3*72);
        b.putInt(dimension).putInt(dimension).putInt(vs.length).putInt(fs.length);
        b.putInt(1).putInt(64).putInt(2).putInt(1).putInt(126).putInt(3).putInt(indexedBytes);
        b.putFloat(0).putFloat(0).putFloat(0).putFloat(1).putFloat(1);
        b.put(vs).put(fs);b.putInt(geometryBytes).putInt(1).putInt(9).putInt(36);
        b.putInt(24).putInt(2).putInt(9).putInt(216);
        b.putInt(0).putInt(106).putInt(0).putInt(1).putInt(106).putInt(12);
        float[] xy={-.75f,-.75f,.75f,-.75f,0,.75f};
        for(int triangle=0;triangle<3;triangle++)for(int vertex=0;vertex<3;vertex++){
            b.putFloat(xy[vertex*2]).putFloat(xy[vertex*2+1]).putFloat(triangle==0?.2f:triangle==1?.8f:.1f);
            b.putFloat(triangle==1?1:0).putFloat(triangle==2?1:triangle==0?.25f:0).putFloat(triangle==0?1:0);
        }
        for(int value:new int[]{0,1,2,3,4,5,6,8,7})b.putInt(value);
        int drawStart=b.position();
        for(int draw=0;draw<3;draw++){
            b.putInt(draw*3).putInt(3);
            float scale=draw==2?.5f:1;
            for(float value:new float[]{scale,0,0,0,0,scale,0,0,0,0,1,0,0,0,0,1})b.putFloat(value);
        }
        for(int[] change:new int[][]{{36,0},{36,17},{24,4},{28,2},{32,129},{60,0x7fc00000},{drawStart,99},{drawStart+4,10}}){
            byte[] invalid=b.array().clone();wrap(invalid).putInt(change[0],change[1]);roundtrip(socket,request(op,invalid),2);
        }
        byte[] pixels=roundtrip(socket,request(op,b.array()),0);
        if(retain){
            require(pixels.length==16,"Snapshot descriptor");ByteBuffer descriptor=wrap(pixels);long id=descriptor.getLong();
            require(descriptor.getInt()==dimension*dimension*4&&descriptor.getInt()==dimension*4,"Snapshot layout");
            roundtrip(socket,request(15,bytes(16).putLong(id).putInt(dimension*dimension*4-4).putInt(8).array()),2);
            roundtrip(socket,request(15,bytes(16).putLong(id).putInt(0).putInt(MAX+1).array()),2);
            pixels=new byte[dimension*dimension*4];
            for(int offset=0;offset<pixels.length;offset+=MAX){int size=Math.min(MAX,pixels.length-offset);
                byte[] chunk=roundtrip(socket,request(15,bytes(16).putLong(id).putInt(offset).putInt(size).array()),0);require(chunk.length==size,"Snapshot chunk size");System.arraycopy(chunk,0,pixels,offset,size);
            }
            roundtrip(socket,request(16,bytes(8).putLong(id).array()),0);
            roundtrip(socket,request(15,bytes(16).putLong(id).putInt(0).putInt(4).array()),4);
        }
        require(pixels.length==dimension*dimension*4,"Scene readback size");
        for(int y=0;y<dimension;y++)for(int x=0;x<dimension;x++){
            float nx=2*(x+.5f)/dimension-1,ny=2*(y+.5f)/dimension-1,side=.375f-.5f*ny-Math.abs(nx);
            boolean inside=ny>-.67f&&ny<.67f&&side>.08f,outside=ny<-.83f||ny>.83f||side<-.08f;if(!inside&&!outside)continue;
            int[] expected={0,inside?64:0,inside?255:0,255};
            for(int k=0;k<4;k++)require(Math.abs((pixels[(y*dimension+x)*4+k]&255)-expected[k])<=1,"Scene depth/cull pixel mismatch x="+x+" y="+y+" component="+k+" actual="+(pixels[(y*dimension+x)*4+k]&255));
            pixelsChecked++;
        }
        require(pixelsChecked==(retain?860488:3330),"Scene sample count: "+pixelsChecked);
    }
    private void sampledTest(LocalSocket socket)throws Exception {
        roundtrip(socket,request(1,new byte[0]),0);
        int total=5592404;
        ByteBuffer meta=bytes(52).putInt(1024).putInt(1024).putInt(11).putInt(0).putInt(0).putInt(0).putInt(2).putInt(2).putInt(2).putFloat(0).putFloat(0).putFloat(10).putInt(total);
        for(int[] change:new int[][]{{0,0},{8,12},{12,9},{24,3},{36,0x7fc00000},{40,0x41300000},{44,0xbf800000},{48,total-4}}){
            byte[] invalid=meta.array().clone();wrap(invalid).putInt(change[0],change[1]);roundtrip(socket,request(19,invalid),2);
        }
        long id=wrap(roundtrip(socket,request(19,meta.array()),0)).getLong();
        roundtrip(socket,request(21,bytes(8).putLong(id+999).array()),4);
        roundtrip(socket,request(21,bytes(8).putLong(id).array()),8);
        roundtrip(socket,request(23,bytes(8).putLong(id).array()),8);
        roundtrip(socket,request(20,bytes(20).putLong(id).putInt(4).putInt(4).putInt(0).array()),2);
        byte[] pixels=new byte[total];int at=0;
        for(int level=0,side=1024;level<11;level++,side=Math.max(1,side/2))for(int y=0;y<side;y++)for(int x=0;x<side;x++){
            pixels[at++]=(byte)(x*17+y*3+level*31);pixels[at++]=(byte)(x*7+y*19+level*23);pixels[at++]=(byte)(x*29+y*11+level*47);pixels[at++]=(byte)255;
        }
        require(at==total,"Mip data total");
        for(int offset=0;offset<total;){int count=Math.min(MAX-16,total-offset);roundtrip(socket,request(20,bytes(16+count).putLong(id).putInt(offset).putInt(count).put(pixels,offset,count).array()),0);offset+=count;}
        roundtrip(socket,request(20,bytes(20).putLong(id).putInt(total).putInt(4).putInt(0).array()),2);
        roundtrip(socket,request(21,bytes(8).putLong(id).array()),0);
        roundtrip(socket,request(21,bytes(8).putLong(id).array()),8);
        roundtrip(socket,request(20,bytes(20).putLong(id).putInt(0).putInt(4).putInt(0).array()),2);
        byte[] vs,fs;
        try(InputStream in=getAssets().open("sampled-scene.vert.spv")){vs=in.readAllBytes();}
        try(InputStream in=getAssets().open("sampled-scene.frag.spv")){fs=in.readAllBytes();}
        boolean reuse=getIntent().getBooleanExtra("reuse",false);
        for(int draw=0;draw<(reuse?4:2);draw++){
            int level=draw%2;
            ByteBuffer b=bytes(8+4+80+64+vs.length+fs.length+144+8);b.putLong(id).putInt(80);
            for(int i=0;i<16;i++)b.putFloat(i%5==0?1:0);b.putFloat(level).putFloat(0).putFloat(0).putFloat(0);
            b.putInt(32).putInt(32).putInt(vs.length).putInt(fs.length).putInt(0).putInt(0).putInt(0).putInt(0).putInt(0).putInt(1).putInt(144);
            // The final reuse draw changes an immutable scene field: it must
            // rebuild, while the earlier changing UBOs must reuse one pipeline.
            b.putFloat(reuse&&draw==3?0:1).putFloat(0).putFloat(1).putFloat(1).putFloat(1);b.put(vs).put(fs);
            b.putInt(104).putInt(1).putInt(6).putInt(24); // Indexed geometry header.
            b.putInt(16).putInt(2).putInt(4).putInt(64); // Vertex stride/attributes/count/bytes.
            b.putInt(0).putInt(103).putInt(0).putInt(1).putInt(103).putInt(8);
            for(float f:new float[]{-1,-1,0,0,1,-1,1,0,1,1,1,1,-1,1,0,1})b.putFloat(f);
            for(int i:new int[]{0,1,2,0,2,3})b.putInt(i);b.putInt(0).putInt(6);
            require(b.position()==b.capacity(),"Sampled scene packet layout");
            ByteBuffer snapshot=wrap(roundtrip(socket,request(23,b.array()),0));long frame=snapshot.getLong();require(snapshot.getInt()==4096&&snapshot.getInt()==128,"Sampled snapshot layout");
            byte[] rgba=roundtrip(socket,request(15,bytes(16).putLong(frame).putInt(0).putInt(4096).array()),0);
            require(rgba.length==4096,"Sampled readback size");
            for(int y=0;y<32;y++)for(int x=0;x<32;x++){
                int side=1024>>level,tx=(int)((x+.5f)*side/32),ty=(int)((y+.5f)*side/32);
                int[] expected={(tx*17+ty*3+level*31)&255,(tx*7+ty*19+level*23)&255,(tx*29+ty*11+level*47)&255,255};
                for(int c=0;c<4;c++)require(Math.abs((rgba[(y*32+x)*4+c]&255)-expected[c])<=1,"Sampled bridge pixel mismatch level="+level+" x="+x+" y="+y);
                pixelsChecked++;
            }
            roundtrip(socket,request(16,bytes(8).putLong(frame).array()),0);
        }
        roundtrip(socket,request(22,bytes(8).putLong(id).array()),0);
        roundtrip(socket,request(22,bytes(8).putLong(id).array()),4);
        // A partial CPU-side upload must also be safely retired on disconnect.
        long partial=wrap(roundtrip(socket,request(19,meta.array()),0)).getLong();
        roundtrip(socket,request(20,bytes(20).putLong(partial).putInt(0).putInt(4).putInt(0).array()),0);
        require(pixelsChecked==(reuse?4096:2048),"Sampled pixel coverage");
    }
    private String bundleHash;
    private int bundleBytes;
    private void bundleTest(LocalSocket socket)throws Exception {
        roundtrip(socket,request(1,new byte[0]),0);
        byte[] metadata=roundtrip(socket,request(9,bytes(8).array()),0);
        require(metadata.length==4,"Bundle metadata");bundleBytes=wrap(metadata).getInt();require(bundleBytes>MAX&&bundleBytes<=4*MAX,"Chunked fixture size");
        roundtrip(socket,request(9,new byte[0]),5);
        for(int[] bad:new int[][]{{bundleBytes,1},{0,MAX+1},{1,0},{-1,1}})roundtrip(socket,request(9,bytes(8).putInt(bad[0]).putInt(bad[1]).array()),2);
        roundtrip(socket,request(9,bytes(4).array()),2);
        java.security.MessageDigest digest=java.security.MessageDigest.getInstance("SHA-256");
        for(int offset=0;offset<bundleBytes;){int n=Math.min(MAX,bundleBytes-offset);byte[] chunk=roundtrip(socket,request(9,bytes(8).putInt(offset).putInt(n).array()),0);require(chunk.length==n,"Exact bundle slice");digest.update(chunk);offset+=n;}
        StringBuilder hash=new StringBuilder();for(byte v:digest.digest())hash.append(String.format(java.util.Locale.ROOT,"%02x",v&255));bundleHash=hash.toString();
    }
    private String run(){
        JSONObject report=new JSONObject();AtomicReference<Throwable> serverFailure=new AtomicReference<>();AtomicReference<String> nativeReport=new AtomicReference<>();Thread serverThread=null;
        try(InputStream shader=getAssets().open("verify.comp.spv")){
            final byte[] trusted=shader.readAllBytes();String name="org.mavk.probe.bridge."+System.nanoTime();
            try(LocalServerSocket listener=new LocalServerSocket(name)){
                serverThread=new Thread(()->{long owner=0;
                    try(LocalSocket peer=listener.accept()){
                        require(peer.getPeerCredentials().getUid()==android.os.Process.myUid(),"Only same-UID harness peer accepted");peer.setSoTimeout(5000);
                        if(getIntent().getBooleanExtra("descriptor",false)){
                            try(android.os.ParcelFileDescriptor channel=android.os.ParcelFileDescriptor.dup(peer.getFileDescriptor())){
                                nativeReport.set(org.mavk.bridge.BridgeEndpoint.serve(channel,trusted,getIntent().getBooleanExtra("validation",false)).toString());
                            }
                        }else{
                            owner=createBridge(trusted,getIntent().getBooleanExtra("validation",false));
                            for(int i=0;i<128;i++){
                                byte[] request;try{request=readFrame(peer.getInputStream());}catch(EOFException eof){break;}
                                byte[] response=exchangeBridge(owner,request);OutputStream out=peer.getOutputStream();for(int offset=0;offset<response.length;){int n=Math.min(4093,response.length-offset);out.write(response,offset,n);offset+=n;}
                            }
                        }
                    }catch(Throwable e){serverFailure.set(e);}finally{if(owner!=0)nativeReport.set(finishBridge(owner));}
                },"mavk-bridge-server");serverThread.start();
                try(LocalSocket client=new LocalSocket()){client.connect(new LocalSocketAddress(name));client.setSoTimeout(5000);if(getIntent().getBooleanExtra("bundle",false))bundleTest(client);else if(getIntent().getBooleanExtra("texture",false))sampledTest(client);else if(getIntent().getBooleanExtra("scene",false))sceneTest(client);else if(getIntent().getBooleanExtra("vertices",false))vertexTest(client);else test(client);}
                serverThread.join(15000);require(!serverThread.isAlive(),"Server teardown timeout");require(serverFailure.get()==null,"Server failed: "+serverFailure.get());
            }
            JSONObject nativeResult=new JSONObject(nativeReport.get());require(nativeResult.getBoolean("core_live_zero_after_disconnect"),"Resource leak after disconnect");require(nativeResult.getInt("gpu_submissions")== (getIntent().getBooleanExtra("bundle",false)?0:getIntent().getBooleanExtra("texture",false)?(getIntent().getBooleanExtra("reuse",false)?5:3):(getIntent().getBooleanExtra("vertices",false)||getIntent().getBooleanExtra("scene",false))?1:16),"GPU submission count");require(nativeResult.getInt("validation_errors")==0,"Bridge validation errors");
            if(getIntent().getBooleanExtra("descriptor",false)){require("BRIDGE_DESCRIPTOR_SESSION_PASS".equals(nativeResult.getString("result"))&&nativeResult.getBoolean("clean_eof")&&nativeResult.getInt("commands")==commands,"Descriptor session completion");}
            report.put("native",nativeResult);report.put("commands",commands);report.put("negative_tests",negativeTests);report.put("words_verified",wordsChecked);report.put("transport","same-UID AF_UNIX stream, separate client/server threads, fragmented writes");report.put("scope","Trusted compute slice only, not guest ICD or graphics bridge");report.put("result","BRIDGE_LOCAL_GPU_COMPUTE_PASS");
            if(getIntent().getBooleanExtra("vertices",false)){report.put("pixels_verified",pixelsChecked);report.put("scope","Guest-supplied shader/vertex data on native Mali; not Windows ICD runtime PASS");report.put("result","BRIDGE_LOCAL_GPU_VERTEX_PASS");}
            if(getIntent().getBooleanExtra("indexed",false)){report.put("result","BRIDGE_LOCAL_GPU_INDEXED_PASS");report.put("scope","Native indexed draw with unused junk vertex0 and nonsequential indices3,1,2; not Windows staging/API PASS");}
            if(getIntent().getBooleanExtra("matrix",false)){report.put("result","BRIDGE_LOCAL_GPU_MATRIX_PASS");report.put("scope","Native indexed draw with nonidentity64B vertex-only matrix; not Windows ICD/application PASS");}
            if(getIntent().getBooleanExtra("scene",false)){report.put("pixels_verified",pixelsChecked);report.put("result","BRIDGE_LOCAL_GPU_SCENE_PASS");report.put("scope","Three indexed draws,distinct push snapshots,D32 depth occlusion and clockwise/backface culling; not Windows ICD/application PASS");}
            if(getIntent().getBooleanExtra("snapshot",false)){report.put("result","BRIDGE_LOCAL_GPU_SNAPSHOT_1024_PASS");report.put("scope","1024 squared scene rendered on Mali,CPU readback snapshot retrieved in four bounded chunks and retired; not zero-copy/persistent GPU image or Windows API PASS");}
            if(getIntent().getBooleanExtra("texture",false)){require(nativeResult.getInt("validation_warnings")==0,"Texture validation warnings");report.put("pixels_verified",pixelsChecked);report.put("uploaded_bytes",5592404);report.put("mip_levels",11);report.put("result","BRIDGE_LOCAL_GPU_PERSISTENT_TEXTURE_PASS");report.put("scope","Actual1024 RGBA full mip chain,fragmented bounded upload,one native upload reused by2 textured UBO draws,explicit and disconnect teardown. NOT Windows ICD/app/performance PASS.");}
            if(getIntent().getBooleanExtra("texture",false)&&getIntent().getBooleanExtra("reuse",false)){require(nativeResult.getInt("pipeline_creates")==3,"Expected compute+two graphics pipelines, not one pipeline per frame");report.put("result","BRIDGE_LOCAL_GPU_RETAINED_RENDERER_PASS");report.put("scope","Four UBO textured draws: first three reuse one graphics pipeline, changed immutable scene rebuilds, texture destruction invalidates cache; partial upload retired at disconnect. NOT guest/visible FPS.");}
            if(getIntent().getBooleanExtra("bundle",false)){require(nativeResult.getInt("validation_warnings")==0,"Bundle validation warnings");report.put("archive_bytes",bundleBytes);report.put("archive_sha256",bundleHash);report.put("result","BRIDGE_LOCAL_FIXED_ARCHIVE_CHUNKS_PASS");report.put("scope","Fixed build archive, bounded chunks and6 invalid requests; NOT guest delivery or Vulkan application PASS.");}
        }catch(Throwable error){try{report.put("result","FAIL");report.put("error",error.toString());}catch(JSONException ignored){}}
        try{Files.writeString(new File(getFilesDir(),"bridge-probe.json").toPath(),report.toString());}catch(IOException e){return "REPORT_WRITE_FAILED: "+e;}return report.toString();
    }
    @Override public void onCreate(Bundle saved){super.onCreate(saved);TextView status=new TextView(this);status.setPadding(40,60,40,40);status.setTextColor(0xffe5eeee);status.setBackgroundColor(0xff091016);status.setText("Mavk Bridge — bounded command/resource/GPU correctness test…");setContentView(status);new Thread(()->{String result=run();runOnUiThread(()->{if(!isDestroyed())status.setText(result);});},"mavk-bridge-client").start();}
}
