import com.droiddeck.launcher.wayland.FrameLoop;
import kotlin.Unit;

public final class FrameLoopCheck {
    int queued, rendered;
    boolean stopOnRender;
    final FrameLoop loop;

    FrameLoopCheck() {
        loop = new FrameLoop(
            () -> { queued++; return Unit.INSTANCE; },
            () -> { queued = 0; return Unit.INSTANCE; },
            time -> render(time));
    }

    Unit render(long time) {
        rendered++;
        if (stopOnRender) loop.stop();
        return Unit.INSTANCE;
    }

    void tick(long time) {
        check(queued == 1, "exactly one scheduled callback");
        queued--;
        loop.onFrame(time);
    }

    static void check(boolean result, String what) {
        if (!result) throw new AssertionError(what);
    }

    public static void main(String[] args) {
        FrameLoopCheck h = new FrameLoopCheck();
        for (int i = 0; i < 10; i++) h.loop.start();
        h.tick(1);
        check(h.rendered == 1 && h.queued == 1, "idempotent start");
        for (int i = 0; i < 100; i++) { h.loop.stop(); h.loop.start(); }
        for (int i = 0; i < 60; i++) h.tick(i + 2);
        check(h.rendered == 61 && h.queued == 1, "rapid detach/attach");
        h.loop.stop();
        h.loop.onFrame(1000);
        check(h.rendered == 61 && h.queued == 0, "ignore stopped callback");
        h.loop.stop();
        check(h.queued == 0, "idempotent stop");
        h.stopOnRender = true;
        h.loop.start();
        h.tick(1001);
        check(h.rendered == 62 && h.queued == 0, "stop during callback");
        h.stopOnRender = false;
        h.loop.start();
        h.tick(1002);
        check(h.rendered == 63 && h.queued == 1, "restart after stop");
        System.out.println("FRAME_LOOP_PASS cases=6 rapidCycles=100");
    }
}
