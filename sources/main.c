
	/*
	
		2.5 brawl, first person nezet verekedes kozben lent jobbra
		labfej tapadjon foldre
		ne sebezzen a hulla amig viszed
		rombolhato tereptargyak
		intersection detection egyszerusitese

	*/

    #ifdef __cplusplus

    // Steam

    #ifdef STEAM

    #include "../projects/steam/CInventory.hpp"

    #endif

    extern "C"
    {
    #endif

    // OpenGL

    // for GL_RENDERBUFFER

	#ifdef IOS
    #import <OpenGLES/ES2/gl.h>
    #import <OpenGLES/ES2/glext.h>
    #elif defined OSX
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    #elif defined ANDROID
    #include <EGL/egl.h>
    #include <GLES/gl.h>
    #elif defined __linux__
    #include <GL/gl.h>
    #include <GL/glu.h>
    #elif defined WINDOWS
    #define GLEW_STATIC
    #include "GL/glew.h"
    #endif

    #if defined(IOS)

    #include <OpenGLES/ES1/gl.h>
    #include <OpenGLES/ES1/glext.h>

    #elif defined(ANDROID)

    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #endif

    #include <time.h>
    #include <stdio.h>
    #include <string.h>

    #include "SDL.h"
    #include "SDL_mixer.h"

    #include "view.c"
    #include "scene.c"
    #include "audio.c"
    #include "bridge.h"
    #include "defaults.c"

    #include "framework/tools/cmd.c"
    #include "framework/core/mtbus.c"
    #include "framework/core/mtcstr.c"
    #include "framework/tools/settings.c"


	char drag = 0;
	char quit = 0;
    float scale = 1.0;

    int32_t width = 700;
    int32_t height = 450;

	float fticks = 0;
    uint32_t prevticks;

    SDL_Window* window;
    SDL_GLContext context;


    void main_onmessage( const char* name ,	void* data )
    {

        if ( strcmp( name , "DONATE") == 0 )
        {

            #ifdef STEAM
            steam_buy( ( char* ) data );
            #else
            bridge_buy( ( char* ) data  );
            #endif

        }
        else if ( strcmp( name , "FEEDBACK") == 0 )
        {

            bridge_open( ( char* ) "http://www.milgra.com/brawl.html" );

        }
        else if ( strcmp( name , "HOMEPAGE") == 0 )
        {

            bridge_open( ( char* ) "http://www.milgra.com" );

        }
        else if ( strcmp( name , "FULLSCREEN") == 0 )
        {

            int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

            char fullscreen = SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN_DESKTOP;

            if ( fullscreen == 1 )
            {

            	SDL_SetWindowFullscreen( window , flags );

			}
            else
            {

             	SDL_SetWindowFullscreen( window , flags | SDL_WINDOW_FULLSCREEN_DESKTOP );

			}

        }
        else if ( strcmp( name , "RESET") == 0 )
        {

			// TODO check this
            defaults_reset( );

        }
        else if ( strcmp( name , "EXIT") == 0 )
        {

			quit = 1;

        }
        else if ( strcmp( name , "EFFECTS" ) == 0 )
        {

            defaults.effects_level += 1;
            if ( defaults.effects_level == 3 ) defaults.effects_level = 0;
            defaults_save( );

        }
        else if ( strcmp( name , "RESETGAME" ) == 0 )
        {

			defaults.sceneindex = 0;
			defaults_save( );

			mtbus_notify( "SCN" , "LOAD" , &defaults.sceneindex );

        }
        else if ( strcmp( name , "RESTARTGAME" ) == 0 )
        {

			defaults.sceneindex = 1;
			defaults_save( );

			mtbus_notify( "SCN" , "LOAD" , &defaults.sceneindex );

        }
        else if ( strcmp( name , "RESETLEVEL" ) == 0 )
        {

			scene.state = kSceneStateAlive;
			mtbus_notify( "SCN" , "LOAD" , &defaults.sceneindex );

			if ( scene.herogroup->bubble == NULL )
			{

				const char* strings[] = {
				"Always block attacks!",
				"Winners always block!!!",
				"Use Up/Down to switch directions at junctions",
				"Be fearless!!!",
				};

				const char* string = strings[ rand() % 4 ];
				scene.herogroup->currenttext = mtstr_frombytes( ( char* ) string );

			}

        }
        else if ( strcmp( name , "NEXTLEVEL" ) == 0 )
        {

			actor_savestate( ( actor_t* ) scene.herogroup->actor );

			if ( defaults.sceneindex + 1 < 7 )
			{

				defaults.sceneindex += 1;
				defaults_save( );

				scene.state = kSceneStateAlive;

				mtbus_notify( "SCN" , "LOAD" , &defaults.sceneindex );

			}
			else
			{

				mtbus_notify( "VIEW" , "SHOWELEMENT" , ( char* )"completedelement" );

			}

        }

    }


    void main_init(	void )
    {

        srand( ( unsigned int ) time( NULL ) );

        /* paths first */

        char* basepath = SDL_GetPrefPath( "milgra" , "brawl" );
        char* respath = mtcstr_fromformat( "%s../resources/" , SDL_GetBasePath( ) , NULL);

	printf( "respath %s\n", respath );
	
        #ifdef ANDROID
        respath = "";
        #endif

		/* message bus */

        mtbus_init( );
        mtbus_subscribe( "CTL" , main_onmessage );

        int framebuffer = 0;
        int renderbuffer = 0;

        glGetIntegerv( GL_FRAMEBUFFER_BINDING , &framebuffer );
        glGetIntegerv( GL_RENDERBUFFER_BINDING , &renderbuffer );

		defaults_init( basepath , respath );

		defaults.scale = scale;
		defaults.width = width;
		defaults.height = height;

		char* fontpath = mtcstr_fromformat( "%s/Impact.ttf" , defaults.respath , NULL );
		defaults.font = font_alloc( fontpath );
        mtmem_release( fontpath );

        /* bridges should be inited after defaults because scaling is set up there */

        #ifdef STEAM
        steam_init();
        #endif

		/* request donation prices from app store, init text scaling, init glfw */

        bridge_init( );

        /* build up view */

		scene_init( );
		view_init( );
        audio_init( defaults.respath );

        /* update projection matrices */

		v2_t dimensions =
		{
			.x = width * scale ,
			.y = height * scale
		};

        mtbus_notify( "SCN" , "RESIZE" , &dimensions );
        mtbus_notify( "VIEW" , "RESIZE" , &dimensions );

        /* load saved level */

        if ( defaults.sceneindex > 0 )
        {

        	mtbus_notify( "VIEW" , "HIDEELEMENT" , ( void* ) "paramelement" );

		}

        mtbus_notify( "SCN" , "LOAD" , &defaults.sceneindex );
		mtbus_notify( "VIEW" , "SHOWELEMENT" , ( void* ) "menuelement" );

		/* cleanup */

        SDL_free( basepath );
        #ifndef ANDROID
        SDL_free( respath );
        #endif

    }


    void main_free(	void )
    {

		audio_free( );
		view_free( );
        scene_free( );
        bridge_free( );
        mtbus_free();
        defaults_free( );

        #ifdef STEAM
        SteamAPI_Shutdown();
        #endif

    }


    void main_loop(	void )
    {

        SDL_Event event;

        while( !quit )
        {

            while( SDL_PollEvent( &event ) != 0 )
            {

				#if defined(IOS) || defined(ANDROID)
				if( event.type == SDL_FINGERDOWN )
				{

					printf( "FINGERDOWN %lld\n" , event.tfinger.fingerId );

					char strid[10];

					snprintf( strid , 10 , "%lld" , event.tfinger.fingerId );

					touch_t touch =
					{
						.id = strid ,
						.x = event.tfinger.x * width * scale ,
						.y = event.tfinger.y * height * scale
					};

					mtbus_notify( "VIEW" , "TOUCHDOWN" , &touch );

				}
				else if( event.type == SDL_FINGERUP )
				{

					printf( "FINGERUP %lld\n", event.tfinger.fingerId );

					char strid[10];

					snprintf( strid , 10 , "%lld" , event.tfinger.fingerId );

					touch_t touch =
					{
						.id = strid ,
						.x = event.tfinger.x * width * scale ,
						.y = event.tfinger.y * height * scale
					};

					mtbus_notify( "VIEW" , "TOUCHUP" , &touch );

				}
				#else
                if( event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION  )
                {

                    int x = 0;
                    int y = 0;

                    SDL_GetMouseState( &x , &y );

                    touch_t touch =
					{
						.id = "mouse" ,
						.x = x * scale ,
						.y = y * scale
					};

                    if ( event.type == SDL_MOUSEBUTTONDOWN )
                    {

                    	drag = 1;
                        mtbus_notify( "VIEW" , "TOUCHDOWN" , &touch );

                    }
                    else if ( event.type == SDL_MOUSEBUTTONUP )
                    {

                    	drag = 0;
                        mtbus_notify( "VIEW" , "TOUCHUP" , &touch );

                    }
                    else if ( event.type == SDL_MOUSEMOTION && drag == 1 )
                    {

                        mtbus_notify( "VIEW" , "TOUCHMOVE" , &touch );

                    }

                }
                #endif
                else if ( event.type == SDL_WINDOWEVENT )
                {

                    if ( event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
                    {

                        width  = event.window.data1;
                        height = event.window.data2;

                        v2_t dimensions =
                        {
                        	.x = width * scale ,
                        	.y = height * scale
						};

						defaults.width = dimensions.x;
						defaults.height = dimensions.y;

                        mtbus_notify( "VIEW" , "RESIZE" ,	&dimensions );
                        mtbus_notify( "SCN" , "RESIZE" , &dimensions );

                    }

                }
                else if ( event.type == SDL_KEYDOWN )
                {

                    mtbus_notify( "SCN" , "KEYDOWN" , &event.key.keysym.sym );

                }
                else if ( event.type == SDL_KEYUP )
                {

                    switch( event.key.keysym.sym )
                    {

                        case SDLK_ESCAPE :

                            main_onmessage( ( char* ) "FULLSCREEN", NULL );
                            break;

                    }

                    mtbus_notify( "SCN" , "KEYUP" , &event.key.keysym.sym );

                }
                else if ( event.type == SDL_APP_WILLENTERFOREGROUND )
                {

                    mtbus_notify( "SCN" , "RESETTIME" , NULL );
    				mtbus_notify( "SND" , "FOREGROUND" , NULL );

                }
                else if ( event.type == SDL_APP_WILLENTERBACKGROUND )
                {

    				mtbus_notify( "SND" , "BACKGROUND" , NULL );

                }
                else if ( event.type == SDL_QUIT )
                {

                    quit = 1;

                }

            }

            // update simulation

            uint32_t ticks = SDL_GetTicks( );

            // avoid first iteration ( ticks == 0 ) or type overflow

            if ( prevticks > 0 && prevticks < ticks )
            {

                int32_t delta = ticks - prevticks;

                float ratio = ( float ) delta / 16.0;

                /* check overflow */

                if ( ratio < 0.99 ) fticks += ratio;
                else fticks += 1.0;
                if ( ratio > 2.0 ) ratio = 2.0;

                defaults.ticks = ( int ) fticks;

                mtbus_notify( "SCN" , "UPDATE" , &ratio );
                mtbus_notify( "VIEW" , "UPDATE" , &ratio );

        		glClear( GL_COLOR_BUFFER_BIT );

                mtbus_notify( "SCN" , "RENDER" , &ticks );
                mtbus_notify( "VIEW" , "RENDER" , &ticks );

            }

            prevticks = ticks;

            SDL_GL_SwapWindow( window );

            #ifdef STEAM
            SteamAPI_RunCallbacks();
            #endif
        }

    }


    int main ( int argc , char * argv [ ] )
    {

        // enable high dpi

        SDL_SetHint( SDL_HINT_VIDEO_HIGHDPI_DISABLED , "0" );

        // init audio

        if ( SDL_Init( SDL_INIT_AUDIO ) != 0 )
        {

        	printf( "SDL Audio init error %s\n" , SDL_GetError() );

		}

		Uint16 audio_format = AUDIO_S16SYS;
		int audio_rate = 44100;
		int audio_channels = 1;
		int audio_buffers = 4096;

		if ( Mix_OpenAudio( audio_rate , audio_format , audio_channels , audio_buffers ) != 0 )
		{

			fprintf( stderr , "Unable to initialize audio: %s\n" , Mix_GetError( ) );

		}

        // init sdl

        if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) == 0 )
        {

            // setup opengl version

            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION , 2 );
            SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION , 1 );
            SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER , 1 );
            SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE , 24 );

            // window size should be full screen on phones, scaled down on desktops

            SDL_DisplayMode displaymode;

            SDL_GetCurrentDisplayMode( 0 , &displaymode );

            if ( displaymode.w < 800 || displaymode.h < 400 )
            {

                width = displaymode.w;
                height = displaymode.h;

            }
            else
            {

                width = displaymode.w * 0.8;
                height = displaymode.h * 0.8;

            }

            #if defined(IOS) || defined(ANDROID)
            width = displaymode.w;
            height = displaymode.h;
            #endif

            // create window

            window = SDL_CreateWindow( "Brawl" , SDL_WINDOWPOS_UNDEFINED ,	SDL_WINDOWPOS_UNDEFINED , width , height , SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
										#if defined(IOS) || defined(ANDROID)
										| SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN_DESKTOP
										#endif
										#ifdef RASPBERRY
										| SDL_WINDOW_RESIZABLE
										#endif
										);

            if ( window != NULL )
            {

                // create context

                context = SDL_GL_CreateContext( window );

                if( context != NULL )
                {

		  
		  GLint GlewInitResult = glewInit();
		  if (GLEW_OK != GlewInitResult) 
		    {
		      printf("ERROR: %s",glewGetErrorString(GlewInitResult));
		    }
		  else printf("GLEW OKAY\n");

		  // calculate scaling

                    int nw;
                    int nh;

                    SDL_GL_GetDrawableSize(	window , &nw , &nh );

                    scale = nw / width;

                    // try to set up vsync

                    if ( SDL_GL_SetSwapInterval( 1 ) < 0 )
                    {

                    	printf( "SDL swap interval error %s\n" , SDL_GetError( ) );

					}

                    main_init( );
                    main_loop( );
                    main_free( );

                    // cleanup

                    SDL_GL_DeleteContext( context );

                }
                else printf( "SDL context creation error %s\n" , SDL_GetError( ) );

                // cleanup

                SDL_DestroyWindow( window );

            }
            else printf( "SDL window creation error %s\n" , SDL_GetError( ) );

            // cleanup

            SDL_Quit( );

        }
        else printf( "SDL init error %s\n" , SDL_GetError( ) );

    	Mix_CloseAudio( );

        return 0;

    }

    #ifdef __cplusplus
    }
    #endif
