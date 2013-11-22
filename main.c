#include <X11/X.h>
#include <sys/time.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>

#define XRES 1280
#define YRES 720

//----------------------------------------------------------------

static const char *vsh = \
    "varying vec4 q;"
    "void main(void)"
    "{"
    "gl_Position = gl_Vertex;"
    "q = vec4(gl_Vertex.xy,gl_MultiTexCoord0.xy);"
    "}";

static const char *fsh = \
    "varying vec4 q;"
    "void main(void)"
      "{"
      "vec2 z = q.xy*vec2(1.7,1.0);"
      // animate
      "vec2 c = vec2(0.5*cos(0.05*q.z) - 0.25*cos(0.1*q.z),"
      "0.5*sin(0.05*q.z) - 0.25*sin(0.1*q.z))*1.01;"
      // julia set!
      "float n;"
      "for( n=0; n<256.0 && dot(z,z)<10.0; n++ )"
      "z = vec2(z.x*z.x-z.y*z.y, 2.0*z.x*z.y) + c;"
      // smooth iterations
      "n -= log(.5*log(dot(z,z)))/log(2.0);"
      "gl_FragColor=vec4(1.0-n/256.0);"
      "}";

//----------------------------------------------------------------

static int doubleBufferVisual[] = { GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

static long timeGetTime( void )
{
    struct timeval now, res;

    gettimeofday(&now, 0);

    return (long)((now.tv_sec*1000) + (now.tv_usec/1000));
}

int main( void )
//void _start()
{
    Display *hDisplay = XOpenDisplay( NULL );
    if( !hDisplay )return 0;

    //if( !glXQueryExtension( hDisplay, 0, 0 ) ) return 0;

    XVisualInfo *visualInfo = glXChooseVisual( hDisplay, DefaultScreen(hDisplay), doubleBufferVisual );
    if( visualInfo == NULL ) return 0;

    GLXContext hRC = glXCreateContext( hDisplay, visualInfo, NULL, GL_TRUE );
    if( hRC == NULL ) return 0;

    XSetWindowAttributes winAttr;
    winAttr.override_redirect = 1;
#if 0
    winAttr.colormap = XCreateColormap( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            visualInfo->visual, AllocNone );
    Window hWnd = XCreateWindow( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            0, 0, XRES, YRES, 0, visualInfo->depth, InputOutput,
            visualInfo->visual, CWColormap|CWOverrideRedirect, &winAttr );
    if( !hWnd ) return 0;
#else
    Window hWnd = XCreateSimpleWindow( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            0, 0, XRES, YRES, 0, 0, 0 );
    if( !hWnd ) return 0;
    XChangeWindowAttributes(hDisplay, hWnd, CWOverrideRedirect, &winAttr);
#endif

    // move cursor out of the windows please
    XWarpPointer(hDisplay, None, hWnd, 0, 0, 0, 0, XRES, 0);

    XMapRaised(hDisplay, hWnd);

    XGrabKeyboard(hDisplay, hWnd, True, GrabModeAsync,GrabModeAsync, CurrentTime);

    glXMakeCurrent( hDisplay, hWnd, hRC );

    const int prId = glCreateProgram();
    if( !prId )return 0;
    const int vsId = glCreateShader( GL_VERTEX_SHADER );
    const int fsId = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( vsId, 1, &vsh, 0 );
    glShaderSource( fsId, 1, &fsh, 0 );
    glCompileShader( vsId );
    glCompileShader( fsId );
    glAttachShader( prId, fsId );
    glAttachShader( prId, vsId );
    glLinkProgram( prId );

    long to = timeGetTime();
    XEvent event;
    while( !XCheckTypedEvent(hDisplay,KeyPress,&event ) )
    {
        glTexCoord1f( 0.001f*(timeGetTime()-to) );
        glUseProgram( prId );
        glRects( -1, -1, 1, 1 );

        glXSwapBuffers( hDisplay, hWnd );
    }
    return 0;
}
