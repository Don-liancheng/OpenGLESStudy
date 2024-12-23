package com.example.triangle;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.core.content.res.ResourcesCompat;

import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class YuvPlayer extends GLSurfaceView implements Runnable, SurfaceHolder.Callback, GLSurfaceView.Renderer {

    public YuvPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
        setRenderer(this);


    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("YuvPlayer","surfaceCreated");
        //在Surface创建的时候启动绘任务
        new Thread(this).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

    }

    @Override
    public void run() {
        //绘制三角形
        //drawTriangle(getHolder().getSurface());
        int[] drawableIds = {
                R.drawable.blue,
                R.drawable.vintagegreen,
                R.drawable.choral,
                R.drawable.pink,
                R.drawable.green,
                R.drawable.violet
                // 添加更多的资源ID...
        };
        ArrayList<Bitmap> bitmaps = loadBitmaps(drawableIds);

        Drawable drawable = ResourcesCompat.getDrawable(getResources(), R.drawable.album, null);
        Bitmap bitmap = ((BitmapDrawable) drawable).getBitmap();
        Surface surface = getHolder().getSurface();  // 从 SurfaceHolder 获取 Surface
        drawTexture(bitmaps,surface);               // 传递正确的对象




    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int i, int i1) {

    }

    @Override
    public void onDrawFrame(GL10 gl10) {

    }

    /**
     * 绘制三角形的native方法
     * @param surface
     */

    public native void drawTriangle(Object surface);

    public native void drawTexture(ArrayList<Bitmap> bitmaps, Object surface);

    // 假设你有一个资源ID的数组或列表
    public ArrayList<Bitmap> loadBitmaps(int[] drawableIds) {
        ArrayList<Bitmap> bitmaps = new ArrayList<>();

        for (int id : drawableIds) {
            Drawable drawable = ResourcesCompat.getDrawable(getResources(), id, null);
            if (drawable instanceof BitmapDrawable) {
                Bitmap bitmap = ((BitmapDrawable) drawable).getBitmap();
                if (bitmap != null) {
                    bitmaps.add(bitmap);
                }
            }
        }

        return bitmaps;
    }

}

