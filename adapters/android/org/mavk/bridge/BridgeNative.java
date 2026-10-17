package org.mavk.bridge;

/** Shared Android binding; neither Activity nor AVF ownership lives in Core. */
public final class BridgeNative {
    static {System.loadLibrary("mavk_probe");}
    private BridgeNative(){}
    public static native long create(byte[] trustedShader,boolean validation);
    public static native byte[] exchange(long session,byte[] frame);
    public static native String finish(long session);
}
