#include "cinder/app/AppBasic.h"
#include "cinder/Cinder.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/Rand.h"
#include "cinder/ArcBall.h"
#include "cinder/Camera.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"

#include "cinder/qtime/QuickTime.h"
#include "cinder/qtime/QuickTimeUtils.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Texture.h"

#include "cinder/audio/Io.h"
#include "cinder/audio/Input.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/PcmBuffer.h"

#include "Resources.h"
#include "cinderSyphon.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <list>
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace audio;
using namespace std;

class AudioVisualizerApp : public AppBasic {

public: 
    void            prepareSettings(Settings *s);   
    void            setup();
    void            audioInit();
    void            fileInit();
    void            openFile();
    void            openFile(GLint which);
    void            loadMovieFile(qtime::MovieGl &mMovie, const fs::path &moviePath, gl::Texture &mFrameTexture);

    void            update();
	void            draw();
    void            drawMovies();
    
#if defined(CINDER_MAC)
    void            drawFft();
#endif

    void            mouseDown(MouseEvent event);
	void            mouseDrag(MouseEvent event);
	void            keyDown(KeyEvent event);
    void            resize(ResizeEvent event);

    syphonServer mScreenSyphon;

private:
    CameraPersp         mCam;
	Arcball             mArcball;
    Quatf               mRotationQuat;
    vector<fs::path>    Images;
	Surface32f          mImage1, mImage2;
	gl::VboMesh         mVboMesh1, mVboMesh2;
    Input               mInput;
    PcmBuffer32fRef     mPcmBuf_Input;
    shared_ptr<float>   mFftDataRef;
    gl::Texture			mTexture1, mTexture2, mFrameTexture1, mFrameTexture2, mFrameTexture3;
	qtime::MovieGl		mMovie1, mMovie2, mMovie3;
    
    GLboolean           LOOPS1, LOOPS2, LINES, MESH1, MESH2, SPIN, VMODE1, VMODE2, FFT1, FFT2, MOVIES_ON1, MOVIES_ON2, MOVIES_ON3, MODE4, quad1, quad2, BLEEP;
    GLfloat             spinX, spinY, spinZ, tiltX, tiltY, tiltZ, angVeloc, mFlash, mAlpha1, mAlpha2, mAlpha3, vAlpha, Yscale1, Yscale2,bandHeight, incr_dec, step1, step2, radius1, radius2, base;
    GLint               mImagePos, mImageSize;
    uint16_t            nBands;
    uint32_t            mMax, mWidth1, mWidth2, mHeight1, mHeight2;
};  

void AudioVisualizerApp::prepareSettings(Settings *s)
{
    s->setWindowSize(1240, 760);
    s->setFrameRate(60.0);
    s->setFullScreen(false);
}

void AudioVisualizerApp::setup()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();    
    gl::enableAlphaBlending();
    
    LOOPS1 = LOOPS2 = SPIN = MESH1 = MESH2 = quad1 = quad2 = LINES = BLEEP = true;
    VMODE1 = VMODE2 = FFT1 = FFT2 = MOVIES_ON1 = MOVIES_ON2 = MOVIES_ON3 = MODE4 = false;
    spinX = spinZ = tiltX = tiltZ = 0.0f;
    spinY = tiltY = 1.0f;
    step1 = step2 = 0.02f;
    radius1 = radius2 = 15.85f;
    vAlpha = mAlpha1 = mAlpha2 = mAlpha3 = angVeloc = mFlash = 0.5f;
    mImagePos = mWidth1 = mWidth2 = mWidth2 = mHeight1 = mHeight2 = mImageSize = 1;
    
    mArcball.setQuat(Quatf(Vec3f(0.0f, 0.977576f, -1.0f), 3.12f));
	//mArcball.setQuat(Quatf(Vec3f(0.0f, 0.1f, 0.0f), 3.68f));
    hideCursor();
   // mScreenSyphon.setName("Screen");
    mScreenSyphon.setName("Arena");
    fileInit();
    mMax = max(mHeight1, mHeight2);
    audioInit();
}

void AudioVisualizerApp::audioInit() 
{   
    incr_dec = 5.0f; //set to 10+ for mic input, else 0.03 for line in *change Yscale1
    Yscale1 = 50.0f; //set to 150-200 for mic in, else 0.03 for line in *change incr_dec
    Yscale2 = - Yscale1;
    bandHeight = 500.0f;
    nBands = 1024;
    base = 5.0f;
    mInput = Input();
    mInput.start();
}

void AudioVisualizerApp::fileInit()
{
    fs::path p("/Users/architechnoiste/Desktop/cinder/samples/AudioVisualizer/xcode/images");
    for(fs::directory_iterator it(p); it != fs::directory_iterator(); ++it) 
         if(!is_directory(*it)) Images.push_back(*it); 
    if(Images.empty()) return; 
    mImageSize = Images.size() - 1;
    openFile();
        
    fs::path mp("/Users/architechnoiste/Desktop/cinder/samples/AudioVisualizer/xcode/video/1.mov");
    if( ! mp.empty() )
        loadMovieFile( mMovie1, mp, mFrameTexture1);
        
    fs::path mp2("/Users/architechnoiste/Desktop/cinder/samples/AudioVisualizer/xcode/video/1.mov");
    if( ! mp2.empty() )
        loadMovieFile( mMovie2, mp, mFrameTexture2);   
        
    fs::path mp3("/Users/architechnoiste/Desktop/cinder/samples/AudioVisualizer/xcode/video/1.mov");
    if( ! mp3.empty() )
        loadMovieFile( mMovie3, mp, mFrameTexture3);   
}

void AudioVisualizerApp::openFile()
{
    if(Images.empty()) return;
    int pos = Rand::randInt() % mImageSize;
    if(pos==0) ++pos;
    mImage1 = loadImage(Images.at(pos));
    mWidth1 = mImage1.getWidth();
    mHeight1 = mImage1.getHeight();
    gl::VboMesh::Layout layout1;
    layout1.setDynamicColorsRGBA();
    layout1.setDynamicPositions();
    if(LINES) {
        if(LOOPS1) { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_POINTS); }
        else       { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_LINE_LOOP); }
    }
    else {
        if(LOOPS1) { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_TRIANGLE_STRIP); }
        else       { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_QUAD_STRIP); }
    }
    mImage2 = loadImage(Images.at(pos));
    mWidth2 = mImage2.getWidth();
    mHeight2 = mImage2.getHeight();
    gl::VboMesh::Layout layout2;
    layout2.setDynamicColorsRGBA();
    layout2.setDynamicPositions();
    if(LINES) {
        if(LOOPS1) { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_POINTS); }
        else       { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_LINE_LOOP); }
    }
    else {
        if(LOOPS1) { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_TRIANGLE_STRIP); }
        else       { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_QUAD_STRIP); }
    }
}

void AudioVisualizerApp::openFile(GLint which)
{
    if(Images.empty()) return;
    int pos;
    ++mImagePos;
    switch(which) {
        case 1: {
            pos = (mImagePos) % mImageSize;
            if(pos==0) ++pos;
            mImage1 = loadImage(Images.at(pos));
            mWidth1 = mImage1.getWidth();
            mHeight1 = mImage1.getHeight();
            gl::VboMesh::Layout layout1;
            layout1.setDynamicColorsRGBA();
            layout1.setDynamicPositions();
            if(LINES) {
                if(LOOPS1) { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_POINTS); }
                else       { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_LINE_LOOP); }
            }
            else {
                if(LOOPS1) { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_TRIANGLE_STRIP); }
                else       { mVboMesh1 = gl::VboMesh(mWidth1 * mHeight1, 0, layout1, GL_QUAD_STRIP); }
            }
        }break;
        case 2: {
            ++mImagePos;
            pos = (mImagePos+1) % mImageSize;
            if(pos==0) ++pos;
            mImage2 = loadImage(Images.at(pos));
            mWidth2 = mImage2.getWidth();
            mHeight2 = mImage2.getHeight();
            gl::VboMesh::Layout layout2;
            layout2.setDynamicColorsRGBA();
            layout2.setDynamicPositions();
            if(LINES) {
                if(LOOPS1) { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_POINTS); }
                else       { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_LINE_LOOP); }
            }
            else {
                if(LOOPS1) { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_TRIANGLE_STRIP); }
                else       { mVboMesh2 = gl::VboMesh(mWidth2 * mHeight2, 0, layout2, GL_QUAD_STRIP); }
            }
        }break;
        default:break;
    }
}

void AudioVisualizerApp::loadMovieFile(qtime::MovieGl &mMovie, const fs::path &moviePath, gl::Texture &mFrameTexture)
{
    try {
        mMovie = qtime::MovieGl( moviePath );
        qtime::initQTVisualContextOptions(300, 300, true);
        mMovie.setLoop();
        mMovie.setVolume(0);
        mMovie.play();
    }
    catch( ... ) {
        console() << "Unable to load the movie." << std::endl;
        mMovie.reset();
    } 
    mFrameTexture.reset();
}

void AudioVisualizerApp::update() {
    if(SPIN) { mRotationQuat.set(Vec3f(spinX, spinY, spinZ), getElapsedSeconds() * angVeloc); }
    else     { mRotationQuat.set(Vec3f(tiltX, tiltY, tiltZ), sin(getElapsedSeconds() * angVeloc)); }
    
    mPcmBuf_Input = mInput.getPcmBuffer();
    if(!mPcmBuf_Input) return;
        
#if defined( CINDER_MAC )
	mFftDataRef = calculateFft(mPcmBuf_Input->getChannelData(CHANNEL_FRONT_LEFT), nBands);
#endif

    if(mMovie1) mFrameTexture1 = mMovie1.getTexture();
    if(mMovie2) mFrameTexture2 = mMovie2.getTexture();
    if(mMovie3) mFrameTexture3 = mMovie3.getTexture();
}

void AudioVisualizerApp::draw()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    if(VMODE1) { 
        GLfloat gray = sin(getElapsedSeconds()) * 0.5f + 0.5f;
        gl::clear(Color(gray, gray, gray ), true);
    }
      else if(VMODE2) { 
        GLfloat r = sin(getElapsedSeconds()) * mFlash;
        GLfloat g = cos(getElapsedSeconds()) * mFlash;
        GLfloat b = tan(getElapsedSeconds()) * mFlash;
        gl::clear(Color(r, g, b), true);
    }
    else { glClearColor(0.0f, 0.0f, 0.0f, 0.0f); }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    gl::pushModelView();
        gl::translate(Vec3f(0.0f, 0.0f, mMax/2.0f));    
        mRotationQuat *= mArcball.getQuat();
        gl::rotate(mRotationQuat);
        
#if defined(CINDER_MAC)
        drawFft();
#endif
        if(quad1) {
        if(mVboMesh1 && MESH1) gl::draw(mVboMesh1);
        if(mVboMesh2 && MESH2) gl::draw(mVboMesh2);
        }
        if(quad2) {
            gl::pushMatrices();
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            if(mVboMesh1 && MESH1) gl::draw(mVboMesh1);
            if(mVboMesh2 && MESH2) gl::draw(mVboMesh2);
            gl::popMatrices();
        }
        gl::color(1.0f, 1.0f, 1.0f, vAlpha);
        drawMovies();
        
    gl::popModelView();
    mScreenSyphon.publishScreen();
    glDisableClientState(GL_VERTEX_ARRAY);
}

#if defined(CINDER_MAC)
void AudioVisualizerApp::drawFft() 
{
    if (!mPcmBuf_Input) return; 
    GLfloat *fftBuf = mFftDataRef.get();
    if(!fftBuf) return;
    GLint scalar = 500;
   
    gl::enableAlphaBlending();
    if(mVboMesh1 && MESH1) {
        Surface32f::Iter pixelIter = mImage1.getIter();
        gl::VboMesh::VertexIter vertexIter(mVboMesh1); 
        for(GLint x=0; x<nBands; ++x) {
            GLfloat y = fftBuf[x] / nBands * bandHeight;
            if(FFT1) {   
                glPushMatrix();
                    glColor4f(randFloat(255.0f), randFloat(255.0f), randFloat(255.0f), mAlpha3);
                    glBegin(GL_QUADS);
                        glVertex3f(x, 0.0f, 0.0f);
                        glVertex3f(x + base, 0.0f, 0.0f);
                        glVertex3f(x + base, y*5, 0.0f);
                        glVertex3f(x, y*5, 0.0f);
                    glEnd();
                    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                    glBegin(GL_QUADS);
                        glVertex3f(x, 0.0f, 0.0f);
                        glVertex3f(x + base, 0.0f, 0.0f);
                        glVertex3f(x + base, y*5, 0.0f);
                        glVertex3f(x, y*5, 0.0f);
                    glEnd();
                    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                    glBegin(GL_QUADS);
                        glVertex3f(x, 0.0f, 0.0f);
                        glVertex3f(x + base, 0.0f, 0.0f);
                        glVertex3f(x + base, y*5, 0.0f);
                        glVertex3f(x, y*5, 0.0f);
                    glEnd();
                    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                    glBegin(GL_QUADS);
                        glVertex3f(x, 0.0f, 0.0f);
                        glVertex3f(x + base, 0.0f, 0.0f);
                        glVertex3f(x + base, y*5, 0.0f);
                        glVertex3f(x, y*5, 0.0f);
                    glEnd();
                glPopMatrix();
            }
            if(BLEEP) {
              GLfloat angle=0.0f;
                while(pixelIter.line() && angle <= pi) {
                    while(pixelIter.pixel()) {
                        ColorA color(pixelIter.r(), pixelIter.g(), pixelIter.b(), mAlpha1);
                        Vec3f map(pixelIter.r(), pixelIter.g(), pixelIter.b());
                        GLfloat height = map.dot(Vec3f(0.3333f, 0.3333f, 0.3333f));
                        GLfloat xmesh = (scalar*cos(angle))/radius1;
                        GLfloat zmesh = (scalar*sin(angle))/radius1;
                        vertexIter.setPosition(xmesh, height * y * Yscale1, zmesh);
                        vertexIter.setColorRGBA(color);
                        ++vertexIter;
                        angle+=step1;   
                    }
                }
            }
            else {
                while(pixelIter.line()) {
                    while(pixelIter.pixel()) {
                        ColorA color(pixelIter.r(), pixelIter.g(), pixelIter.b(), mAlpha1);
                        Vec3f map(pixelIter.r(), pixelIter.g(), pixelIter.b());
                        GLfloat height = map.dot(Vec3f(0.3333f, 0.3333f, 0.3333f));
                        GLfloat xmesh = pixelIter.x() - mWidth1 / 2.0f;
                        GLfloat zmesh = pixelIter.y() - mHeight1 / 2.0f;
                        vertexIter.setPosition(xmesh, height * y * Yscale1, zmesh * 2.0f);
                        vertexIter.setColorRGBA(color);
                        ++vertexIter;
                    }
                }
            } 
        }
    }
    if(mVboMesh2 && MESH2) {
        glPushMatrix();
        glRotatef(90, 1.0f, 0.0f, 0.0f);
        Surface32f::Iter pixelIter = mImage2.getIter();
        gl::VboMesh::VertexIter vertexIter(mVboMesh2);  
        for(GLint x=0; x<nBands; ++x) {
            GLfloat y = fftBuf[x] / nBands * bandHeight;
            if(BLEEP) {
               GLfloat angle = 0.0f;
               while(pixelIter.line() && angle <= pi) {
                    while(pixelIter.pixel()) {
                        ColorA color(pixelIter.r(), pixelIter.g(), pixelIter.b(), mAlpha2);
                        Vec3f map(pixelIter.r(), pixelIter.g(), pixelIter.b());
                        GLfloat height = map.dot(Vec3f(0.3333f, 0.3333f, 0.3333f));
                        GLfloat xmesh = (scalar*cos(angle))/radius2;
                        GLfloat zmesh = (scalar*sin(angle))/radius2;
                        vertexIter.setPosition(xmesh, height * y * Yscale2, zmesh);
                        vertexIter.setColorRGBA(color);
                        ++vertexIter;
                        angle+=step2;   
                    }
                }
            }
            else {
             if(FFT2) {   
                    glPushMatrix();
                        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                        glColor4f(randFloat(255.0f), randFloat(255.0f), randFloat(255.0f), mAlpha3);
                        glBegin(GL_QUADS);
                            glVertex3f(x, 0.0f, 0.0f);
                            glVertex3f(x + base, 0.0f, 0.0f);
                            glVertex3f(x + base, y*5, 0.0f);
                            glVertex3f(x, y*5, 0.0f);
                        glEnd();
                        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                        glBegin(GL_QUADS);
                            glVertex3f(x, 0.0f, 0.0f);
                            glVertex3f(x + base, 0.0f, 0.0f);
                            glVertex3f(x + base, y*5, 0.0f);
                            glVertex3f(x, y*5, 0.0f);
                        glEnd();
                        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                        glBegin(GL_QUADS);
                            glVertex3f(x, 0.0f, 0.0f);
                            glVertex3f(x + base, 0.0f, 0.0f);
                            glVertex3f(x + base, y*5, 0.0f);
                            glVertex3f(x, y*5, 0.0f);
                        glEnd();
                        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                        glBegin(GL_QUADS);
                            glVertex3f(x, 0.0f, 0.0f);
                            glVertex3f(x + base, 0.0f, 0.0f);
                            glVertex3f(x + base, y*5, 0.0f);
                            glVertex3f(x, y*5, 0.0f);
                        glEnd();
                glPopMatrix();
            }
                while(pixelIter.line()) {
                    while(pixelIter.pixel()) {
                        ColorA color(pixelIter.r(), pixelIter.g(), pixelIter.b(), mAlpha2);
                        Vec3f map(pixelIter.r(), pixelIter.g(), pixelIter.b());
                        GLfloat height = map.dot(Vec3f(0.3333f, 0.3333f, 0.3333f));
                        GLfloat xmesh = pixelIter.x() - mWidth2 / 2.0f;
                        GLfloat zmesh = pixelIter.y() - mHeight2 / 2.0f;
                        vertexIter.setPosition(xmesh, height * y * Yscale2, zmesh * 2.0f);
                        vertexIter.setColorRGBA(color);
                        ++vertexIter;
                    }
                }
            }
        } 
        glPopMatrix();
    }
}
#endif

void AudioVisualizerApp::keyDown(KeyEvent event)
{
	switch(event.getChar()) {
        case 'c': hideCursor(); break;
        case 'C': showCursor(); break;
        case 'F': setFullScreen(!isFullScreen()); break;
        case 'Q': exit(0); break;
                 
        case 'v': VMODE2=false; VMODE1=true; break;
        case 'V': VMODE1=false; VMODE2=true; break;
        case 'n': VMODE1 = VMODE2 = false; break;

        case 'i': FFT1 ? FFT1=false : FFT1=true; break;
        case 'I': FFT2 ? FFT2=false : FFT2=true; break;
        case '/': mAlpha3<=0.0f ? mAlpha3=0.0f : mAlpha3-=0.05f; break;
        case '?': mAlpha3+=0.05f; break;
        
        case 'b': BLEEP ? BLEEP=false : BLEEP=true; break;
    
        case '1': MESH1  ?  MESH1=false :  MESH1=true; break;
        case '2': MESH2  ?  MESH2=false :  MESH2=true; break;
        case '3': LOOPS1 ? LOOPS1=false : LOOPS1=true; break;
        case '4': LOOPS2 ? LOOPS2=false : LOOPS2=true; break;
        case '*': LINES ? LINES=false : LINES=true; break;
        
        case '!': MOVIES_ON1 ? MOVIES_ON1=false : MOVIES_ON1=true; break;
        case '@': MOVIES_ON2 ? MOVIES_ON2=false : MOVIES_ON2=true; break;
        case '#': MOVIES_ON3 ? MOVIES_ON3=false : MOVIES_ON3=true; break;
        case '$': { MOVIES_ON1=true; MOVIES_ON2=true; MOVIES_ON3=true; } break;
        case '%': { MOVIES_ON1=false; MOVIES_ON2=false; MOVIES_ON3=false; } break;
        case 'w': base<=0.0f ? base=0.0f : base-=0.1f; break;
        case 'W': base+=0.1f; break;

        case '6': Yscale1 =   abs(Yscale1);  break;
        case '^': Yscale1 = -(abs(Yscale1)); break;
        case '7': Yscale2 =   abs(Yscale2);  break;
        case '&': Yscale2 = -(abs(Yscale2)); break;
        case '{': Yscale1=0.0f; break;
        case '}': Yscale2=0.0f; break; 
        case 'u': incr_dec<=0.0f ? incr_dec=0.0f : incr_dec-=10.0f; break;
        case 'U': incr_dec+=10.0f; break;
        case 'Z': Yscale1+=incr_dec; break;
        case 'z': Yscale1-=incr_dec; break;
        case 'X': Yscale2+=incr_dec; break;
        case 'x': Yscale2-=incr_dec; break;
        case 'M': {Yscale1+=incr_dec; Yscale2-=incr_dec;} break;
        case 'm': {Yscale1-=incr_dec; Yscale2+=incr_dec;} break;
        
        case '8': openFile(1); break;
        case '9': openFile(2); break;
        case '0': openFile();  break;
    
        case '|': mArcball.setQuat(Quatf(Vec3f(0.0f, 0.977576f, -1.0f), 3.12f)); break;
        case ']': mArcball.setQuat(Quatf(Vec3f(0.0f, 0.0f, 0.1f), 3.68f)); break;
        case '[': mArcball.setQuat(Quatf(Vec3f(0.0f, 0.1f, 0.0f), 3.68f)); break;
        case 'p': mArcball.setQuat(Quatf(Vec3f(0.0f, 0.0f, -0.1f), 3.68f)); break;
        case 'P': mArcball.setQuat(Quatf(Vec3f(-0.1f, -0.1f, -0.1f), 3.68f)); break;
        
        case '5': SPIN   ?   SPIN=false :   SPIN=true; break;  
        case '<': case ',': angVeloc<=0.0f ? angVeloc=0.0f : angVeloc-=0.1f; break;
        case '>': case '.': angVeloc+=0.1f; break;
            
        case 'g': mAlpha1 = 0.15f;    break;
        case 'G': mAlpha2 = 0.15f;    break;
		case 'l': mAlpha1 = 0.3333f;  break;
        case 'L': mAlpha2 = 0.3333f;  break;
		case 'h': mAlpha1 = 0.6666f;  break;
        case 'H': mAlpha2 = 0.6666f;  break;
		case 'o': mAlpha1 = 1.0f;     break;
        case 'O': mAlpha2 = 1.0f;     break;
        case '_': vAlpha<=0 ? vAlpha=0.0f : vAlpha-=0.05f; break;
        case '+': vAlpha+=0.05f; break;
        case 'y': vAlpha=0.5f; break;
        case 'Y': vAlpha=1.0f; break;
        case '-': mAlpha1<=0 ? mAlpha1=0.0f : mAlpha1-=0.05f; mAlpha2=mAlpha1; break;
        case '=': mAlpha1+=0.05f; mAlpha2=mAlpha1; break;
        
        case 'j': quad1 ? quad1=false : quad1=true; break;
        case 'k': quad2 ? quad2=false : quad2=true; break;
        
        case 't': radius1+=0.75f; radius2+=0.75f; break;
        case 'T': radius1-=0.75f; radius2-=0.75f; break;
        case 'r': radius1+=0.75f; break;
        case 'R': radius1-=0.75f; break;
        case 'e': radius2+=0.75f; break;
        case 'E': radius2-=0.75f; break;
        
        case 'a': step1-=0.002f; break;
        case 'A': step1+=0.002f; break;
        case 's': step2-=0.002f; break;
        case 'S': step2+=0.002f; break;
        case 'd': step1-=0.002f; step2-=0.002f; break;
        case 'D': step1+=0.002f; step2+=0.002f; break;
        
        default : break;
	}
}

void AudioVisualizerApp::mouseDown(MouseEvent event) { mArcball.mouseDown(event.getPos()); }
void AudioVisualizerApp::mouseDrag(MouseEvent event) { mArcball.mouseDrag(event.getPos()); }

void AudioVisualizerApp::resize( ResizeEvent event )
{
    mArcball.setWindowSize(getWindowSize());
	mArcball.setCenter(Vec2f(getWindowWidth()/2.0f, getWindowHeight()/2.0f));
	mArcball.setRadius(getWindowHeight()/2.0f);
	mCam.lookAt(Vec3f(0.0f, 0.0f, -150), Vec3f::zero());
	mCam.setPerspective(60.0f, getWindowAspectRatio(), 0.1f, 1000.0f);
	gl::setMatrices(mCam);
}

void AudioVisualizerApp::drawMovies() {
    if(mFrameTexture1) {
        if(MOVIES_ON1) {
            Rectf centeredRect = Rectf( mFrameTexture1.getBounds() ).getCenteredFit( getWindowBounds(), true );
            gl::pushMatrices();
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture1, centeredRect  );
            gl::popMatrices();
        } 
    }
    if(mFrameTexture2) { 
        if(MOVIES_ON2) {
            Rectf centeredRect = Rectf( mFrameTexture2.getBounds() ).getCenteredFit( getWindowBounds(), true );            
            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            gl::pushMatrices();
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture2, centeredRect  );
            gl::popMatrices();
        }
    }
    if(mFrameTexture3) {
        if(MOVIES_ON3) {
            Rectf centeredRect = Rectf( mFrameTexture3.getBounds() ).getCenteredFit( getWindowBounds(), true );
            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            gl::pushMatrices();
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
                glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
                gl::draw( mFrameTexture3, centeredRect  );
            gl::popMatrices();
        }
    }
}

CINDER_APP_BASIC( AudioVisualizerApp, RendererGl );
