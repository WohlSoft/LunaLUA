#include <thread>
#include "../Misc/ThreadedCmdQueue.h"
#include "GLEngineProxy.h"
#include "../Defines.h"

// Instance
GLEngineProxy g_GLEngine;

GLEngineProxy::GLEngineProxy() {
    mpThread = NULL;
}

GLEngineProxy::~GLEngineProxy() {
    /*if (mpThread != NULL) {
        GLEngineCmd cmd;
        cmd.mCmd = GLEngineCmd::GL_ENGINE_CMD_EXIT;
        mQueue.push(cmd);
        mpThread->join();
    }*/
}

void GLEngineProxy::Init() {
    if (mpThread == NULL) {
        mpThread = new std::thread( [this] {this->ThreadMain(); });
    }
}

void GLEngineProxy::ThreadMain2() {
    dbgboxA("Foo");
}

void GLEngineProxy::ThreadMain() {
    while (1) {
        GLEngineCmd cmd = mQueue.pop();

        RunCmd(cmd);

        if (cmd.mCmd == GLEngineCmd::GL_ENGINE_CMD_EXIT) return;

        //return;
    }
}

void GLEngineProxy::RunCmd(const GLEngineCmd& cmd) {
    switch (cmd.mCmd) {
    case GLEngineCmd::GL_ENGINE_CMD_CLEAR:
        mGLEngine.ClearTextures();
        break;
    case GLEngineCmd::GL_ENGINE_CMD_BITBLT:
        mGLEngine.EmulatedBitBlt(
            cmd.mData.mBitBlt.nXDest, cmd.mData.mBitBlt.nYDest,
            cmd.mData.mBitBlt.nWidth, cmd.mData.mBitBlt.nHeight,
            cmd.mData.mBitBlt.hdcSrc,
            cmd.mData.mBitBlt.nXSrc, cmd.mData.mBitBlt.nYSrc,
            cmd.mData.mBitBlt.dwRop);
        break;
    case GLEngineCmd::GL_ENGINE_CMD_STRETCHBLT:
        mGLEngine.EmulatedStretchBlt(
            cmd.mData.mStretchBlt.hdcDest,
            cmd.mData.mStretchBlt.nXOriginDest, cmd.mData.mStretchBlt.nYOriginDest,
            cmd.mData.mStretchBlt.nWidthDest, cmd.mData.mStretchBlt.nHeightDest,
            cmd.mData.mStretchBlt.hdcSrc,
            cmd.mData.mStretchBlt.nXOriginSrc, cmd.mData.mStretchBlt.nYOriginSrc,
            cmd.mData.mStretchBlt.nWidthSrc, cmd.mData.mStretchBlt.nHeightSrc,
            cmd.mData.mStretchBlt.dwRop);
        break;
    case GLEngineCmd::GL_ENGINE_CMD_EXIT:
        return;
    default:
        break;
    }
}

void GLEngineProxy::ClearTextures() {
    Init();
    GLEngineCmd cmd;

    cmd.mCmd = GLEngineCmd::GL_ENGINE_CMD_CLEAR;
    mQueue.push(cmd); //ThreadMain();
    mQueue.waitTillEmpty();
    //RunCmd(cmd);
}

void GLEngineProxy::EmulatedBitBlt(int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
    Init();
    GLEngineCmd cmd;

    cmd.mCmd = GLEngineCmd::GL_ENGINE_CMD_BITBLT;
    cmd.mData.mBitBlt.nXDest = nXDest;
    cmd.mData.mBitBlt.nYDest = nYDest;
    cmd.mData.mBitBlt.nWidth = nWidth;
    cmd.mData.mBitBlt.nHeight = nHeight;
    cmd.mData.mBitBlt.hdcSrc = hdcSrc;
    cmd.mData.mBitBlt.nXSrc = nXSrc;
    cmd.mData.mBitBlt.nYSrc = nYSrc;
    cmd.mData.mBitBlt.dwRop = dwRop;

    mQueue.push(cmd); //ThreadMain();
    //RunCmd(cmd);
}

BOOL GLEngineProxy::EmulatedStretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
    HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
    DWORD dwRop)
{
    Init();
    GLEngineCmd cmd;

    cmd.mCmd = GLEngineCmd::GL_ENGINE_CMD_STRETCHBLT;
    cmd.mData.mStretchBlt.hdcDest = hdcDest;
    cmd.mData.mStretchBlt.nXOriginDest = nXOriginDest;
    cmd.mData.mStretchBlt.nYOriginDest = nYOriginDest;
    cmd.mData.mStretchBlt.nWidthDest = nWidthDest;
    cmd.mData.mStretchBlt.nHeightDest = nHeightDest;
    cmd.mData.mStretchBlt.hdcSrc = hdcSrc;
    cmd.mData.mStretchBlt.nXOriginSrc = nXOriginSrc;
    cmd.mData.mStretchBlt.nYOriginSrc = nYOriginSrc;
    cmd.mData.mStretchBlt.nWidthSrc = nWidthSrc;
    cmd.mData.mStretchBlt.nHeightSrc = nHeightSrc;
    cmd.mData.mStretchBlt.dwRop = dwRop;

    mQueue.push(cmd); //ThreadMain();
    //RunCmd(cmd);
    return TRUE;
}

