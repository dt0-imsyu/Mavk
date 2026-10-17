package org.mavk.bridge;

import android.os.ParcelFileDescriptor;
import android.system.Os;
import android.system.OsConstants;
import java.io.*;
import java.nio.*;
import org.json.*;
import java.util.concurrent.*;

/** One bounded session over a caller-owned app-approved duplex descriptor. */
public final class BridgeEndpoint {
    private static final int MAX=1024*1024;
    private BridgeEndpoint(){}
    private static int payloadBytes(byte[] header)throws IOException {
        int n=ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN).getInt(24);
        if(n<0||n>MAX)throw new IOException("Bridge payload bound before allocation");return n;
    }
    // Shared with the local proof; short reads are not message boundaries.
    public static JSONObject serve(InputStream input,OutputStream output,byte[] shader,boolean validation)throws Exception {
        long session=0;int commands=0;long received=0,sent=0;boolean eof=false;
        JSONObject report;Throwable failure=null;
        try {
            session=BridgeNative.create(shader,validation);if(session==0)throw new IOException("Native session unavailable");
            DataInputStream in=new DataInputStream(input);
            for(int i=0;i<128;i++){
                int first=in.read();if(first<0){eof=true;break;}
                byte[] header=new byte[32];header[0]=(byte)first;in.readFully(header,1,31);
                int length=payloadBytes(header);byte[] frame=new byte[32+length];System.arraycopy(header,0,frame,0,32);in.readFully(frame,32,length);
                byte[] response=BridgeNative.exchange(session,frame);if(response==null||response.length<32||response.length>MAX+32)throw new IOException("Invalid native response bound");
                output.write(response);output.flush();commands++;received+=frame.length;sent+=response.length;
            }
            if(!eof)throw new IOException("Bridge command limit reached without disconnect");
        }catch(Throwable error){failure=error;}
        finally {report=session==0?new JSONObject():new JSONObject(BridgeNative.finish(session));}
        report.put("commands",commands);report.put("received_bytes",received);report.put("sent_bytes",sent);report.put("clean_eof",eof);
        boolean healthy=failure==null&&session!=0&&report.optBoolean("core_live_zero_after_disconnect")&&report.optInt("validation_errors",-1)==0;
        report.put("result",healthy?"BRIDGE_DESCRIPTOR_SESSION_PASS":"FAIL");
        if(failure!=null)report.put("error_category",failure.getClass().getSimpleName());
        return report;
    }
    public static JSONObject serve(ParcelFileDescriptor channel,byte[] shader,boolean validation)throws Exception {
        // Owner may shutdown the socket to cancel; merely closing one duplicate
        // does not interrupt the others. These duplicates own their streams.
        // AVF supplies an approved transport FD, not necessarily a socket on
        // which this UID can set options. Bound the entire diagnostic session
        // instead of requiring SO_*TIMEO support on that foreign descriptor.
        ScheduledExecutorService deadline=Executors.newSingleThreadScheduledExecutor(r->{Thread t=new Thread(r,"mavk-channel-deadline");t.setDaemon(true);return t;});
        ScheduledFuture<?> timeout=deadline.schedule(()->{try{Os.shutdown(channel.getFileDescriptor(),OsConstants.SHUT_RDWR);}catch(Exception ignored){}},60,TimeUnit.SECONDS);
        try(InputStream input=new ParcelFileDescriptor.AutoCloseInputStream(ParcelFileDescriptor.dup(channel.getFileDescriptor()));
            OutputStream output=new ParcelFileDescriptor.AutoCloseOutputStream(ParcelFileDescriptor.dup(channel.getFileDescriptor()))){return serve(input,output,shader,validation);}
        finally{timeout.cancel(false);deadline.shutdownNow();}
    }
}
