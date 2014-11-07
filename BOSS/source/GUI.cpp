#include "GUI.h"

#ifdef BOSS_ENABLE_GUI

using namespace BOSS;

#define GUI_INITIAL_WIDTH  1920
#define GUI_INITIAL_HEIGHT 1080

#define BOSS_MAX_TEXTURES 200
#define BOSS_TEXTURE_INTERVAL 64

GUI::GUI() 
    : windowSizeX(GUI_INITIAL_WIDTH)
    , windowSizeY(GUI_INITIAL_HEIGHT)
    , bl(false)
    , br(false)
    , bu(false)
    , bd(false)
    , cameraX(0)
    , cameraY(0)
    , previousMouseX(0)
    , previousMouseY(0)
    , started(false)
    , mousePressed(false)
    , shiftPressed(false)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        BOSS_ASSERT(false, "Unable to initialise SDL");
    }
}

GUI & GUI::Instance()
{
    static GUI g;
    return g;
}

GUI::~GUI()
{
    SDL_Quit();
}

bool GUI::isStarted() const
{
    return started;
}

void GUI::OnStart()
{
    if (started)
    {
        return;
    }

    cameraX   = 0;
    cameraY   = 0;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,1);
    screen = SDL_SetVideoMode(windowSizeX,windowSizeY,32,SDL_OPENGL | SDL_RESIZABLE);
    SDL_WM_SetCaption("BOSS OpenGL Visualization",0);

    LoadTextures();

    // enable alpha blending for transparency
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glViewport(0,0,windowSizeX,windowSizeY);

#ifdef WIN32
    setVSync(0);
#endif

    started = true;
}


void GUI::OnFrame()
{
    BOSS_ASSERT(isStarted(), "Must initialize GUI before calling OnFrame()");

    // Handle input events
    HandleEvents();

    // Render the frame
    glClear(GL_COLOR_BUFFER_BIT);
    Render();

    SDL_GL_SwapBuffers();
}

void GUI::OnResize(SDL_Event & event)
{
    windowSizeX = event.resize.w;
    windowSizeY = event.resize.h;

    glViewport(0,0,windowSizeX,windowSizeY);
    OnFrame();
}

void GUI::HandleEvents()
{
    // Handle SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        const bool pressed(event.key.state == SDL_PRESSED);
        switch (event.type)
        {
            case SDL_VIDEORESIZE:
            {
                OnResize(event);
                break;
            }
            case SDL_MOUSEMOTION:
            {
                if ((previousMouseX != 0 || previousMouseY != 0) && (event.motion.state & SDL_BUTTON_LEFT))
                {
                    //cameraX -= event.motion.xrel;
                    //cameraY -= event.motion.yrel;
                }
                previousMouseX = event.motion.x;
                previousMouseY = event.motion.y;
                break;
            }
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_LSHIFT:
                    shiftPressed = pressed;
                    break;
                case SDLK_p:
                {
                    static int screenshots = 0;
                    std::stringstream ss;
                    ss << "screenshot_" << screenshots++ << ".bmp";
                    std::cout << "Taking screenshot: " << ss.str() << "\n";
                    saveScreenshotBMP(ss.str());
                    break;
                }
                }
                break;
            }
            case SDL_KEYUP:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_LSHIFT:
                    shiftPressed = pressed;
                    break;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                if (event.button.button == SDL_BUTTON_WHEELUP)
                {
                    cameraY -= 40;
                }
                else if (event.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    cameraY += 40;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    //currentState.addCard((event.button.y > (windowSizeY / 2)) ? 1 : 0, CardType(rand() % CardTypes::GetAllCardTypes().size()));
                }
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {

                }
                break;
            }
            case SDL_QUIT:
            {
                std::cerr << "SDL_QUIT caught\n\n";
                exit(0);
                //MCDS::System::FatalError("Visualization Shut Down");
            }
        }
    }
}

void GUI::Render()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        {
            glOrtho(0,windowSizeX,windowSizeY,0,-1,1);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            {
                glTranslatef(static_cast<float>(-cameraX),static_cast<float>(-cameraY),0);
                
                DrawGameState();
                DrawConcurrency();
            }

            glPopMatrix();
        }
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    glPopAttrib();
}

void GUI::DrawConcurrency()
{
    std::vector<int> layers(_startTimes.size(), -1);
    int maxLayer = 0;

    for (size_t i(0); i < _startTimes.size(); ++i)
    {
        FrameCountType start    = _startTimes[i];
        FrameCountType finish   = _finishTimes[i];

        std::vector<int> layerOverlap;
        // loop through everything up to this action and see which layers it can't be in
        for (size_t j(0); j < i; ++j)
        {
            if (start <= _finishTimes[j])
            {
                layerOverlap.push_back(layers[j]);
            }
        }

        // find a layer we can assign to this value
        int layerTest = 0;
        while (true)
        {
            if (std::find(layerOverlap.begin(), layerOverlap.end(), layerTest) == layerOverlap.end())
            {
                layers[i] = layerTest;
                if (layerTest > maxLayer)
                {
                    maxLayer = layerTest;
                }
                break;
            }

            layerTest++;
        }
    }

    
    float maxWidth = 1600;
    float maxFinishTime = 1;
    for (size_t i(0); i < _finishTimes.size(); ++i)
    {
        if (_finishTimes[i] > maxFinishTime)
        {
            maxFinishTime = _finishTimes[i];
        }
    }

    if (_currentState.getCurrentFrame() > maxFinishTime)
    {
        maxFinishTime = _currentState.getCurrentFrame();
    }

    float scale = maxWidth / maxFinishTime;

    // draw it

    GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat white2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 0.8f};
    GLfloat grey[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat red[4] = {0.6f, 0.0f, 0.0f, 1.0f};
    PositionType height = 20;
    PositionType heightBuffer = 3;
    Position concurrent(225, 800);
    float framePos =  (concurrent.x() + scale*_currentState.getCurrentFrame());
    
    GUITools::DrawRect(Position(concurrent.x() - 10, concurrent.y() - 10), Position(concurrent.x() + maxWidth + 10, concurrent.y() + (maxLayer)*(height + heightBuffer) + 10 + height), grey);

    

    for (size_t i(0); i < layers.size(); ++i)
    {
        Position topLeft(concurrent.x() + _startTimes[i]*scale, concurrent.y() + (height + heightBuffer) * layers[i]);
        Position bottomRight(topLeft.x() + (_finishTimes[i] - _startTimes[i])*scale, topLeft.y() + height);

        std::string name = _buildOrder[i].getName();
        size_t loc = name.find(" ");
        if (loc != std::string::npos)
        {
            name = name.substr(loc);
        }

        GUITools::DrawRect(topLeft, bottomRight, red);
        GUITools::DrawStringWithShadow(topLeft + Position(2, 13), name, white);
    }

    GUITools::DrawLine(Position((PositionType)framePos, concurrent.y() - 10), Position((PositionType)framePos, concurrent.y() + (maxLayer)*(height + heightBuffer) + 10 + height), 2, white);
}

void GUI::DrawGameState()
{
    GLfloat green[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat greyx[4] = {0.7f, 0.7f, 0.7f, 1.0f};
    const RaceID race = _currentState.getRace();
    const std::vector<ActionType> & allActions = ActionTypes::GetAllActionTypes(race);
    static const ActionType & Larva = ActionTypes::GetActionType("Zerg_Larva");
    static const ActionType & Hatchery = ActionTypes::GetActionType("Zerg_Hatchery");

    
    Position bopos(0,20);
    for (size_t i(0); i < _buildOrder.size(); ++i)
    {
        std::stringstream ss;
        ss << ((i == _boIndex) ? " > " : "   ") << _buildOrder[i].getName();
        GUITools::DrawStringLargeWithShadow(bopos + Position(0, 15*i), ss.str(), i < _boIndex ? green : greyx);
    }


    std::stringstream ssres;
    ssres << "Frame:    " << _currentState.getCurrentFrame() << " " << _currentState.getCurrentFrame()/24 <<"\n";
    ssres << "Minerals: " << (int)_currentState.getMinerals() << "\n";
    ssres << "MWorkers: " << (int)_currentState.getNumMineralWorkers() << "\n";
    ssres << "Gas:      " << (int)_currentState.getGas() << "\n";
    ssres << "GWorkers: " << (int)_currentState.getNumGasWorkers() << "\n";
    ssres << "C Supply: " << (int)_currentState.getUnitData().getCurrentSupply() << "\n";
    ssres << "M Supply: " << (int)_currentState.getUnitData().getMaxSupply() << "\n";
    GUITools::DrawStringLargeWithShadow(Position(225, 45), ssres.str(), white);


    const size_t width = 64;
    const size_t cwidth = 96;
    Position completed(400,0);
    
    if (_currentState.getRace() == Races::Zerg)
    {
        const UnitCountType & numLarva = _currentState.getHatcheryData().numLarva();

        DrawActionType(Larva, completed, cwidth);
        std::stringstream num; num << numLarva << "\n" << (Constants::ZERG_LARVA_TIMER-(_currentState.getCurrentFrame() % Constants::ZERG_LARVA_TIMER));
        GUITools::DrawString(completed + Position(10, 20), num.str(), white);
        completed.add(cwidth, 0);
    }

    for (size_t a(0); a < allActions.size(); ++a)
    {
        const size_t numCompleted = _currentState.getUnitData().getNumCompleted(allActions[a]);

        if (numCompleted > 0)
        {
            DrawActionType(allActions[a], completed, cwidth);
            std::stringstream num; num << numCompleted;
            GUITools::DrawStringLargeWithShadow(completed + Position(10, 20), num.str(), white);
            completed.add(cwidth, 0);
        }
    }

    Position legal(400,125);
    ActionSet legalActions;
    _currentState.getAllLegalActions(legalActions);
    GUITools::DrawStringLargeWithShadow(legal + Position(0,-5), "Legal Actions", white);
    for (size_t a(0); a < legalActions.size(); ++a)
    {
        const ActionType & action = legalActions[a];
        DrawActionType(legalActions[a], legal, width/2);
        legal.add(width/2, 0);
    }

    GLfloat grey2[4] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat grey[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    Position progress(225, 200);
    Position progressBar(200,32);
    Position progressBuffer(100, 16);
    GUITools::DrawStringLargeWithShadow(progress, "Actions in Progress", white);
    for (size_t a(0); a < _currentState.getUnitData().getNumActionsInProgress(); ++a)
    {
        size_t index = _currentState.getUnitData().getNumActionsInProgress() - a - 1;

        const ActionType actionInProgress = _currentState.getUnitData().getActionInProgressByIndex(index);
        const FrameCountType finishTime = _currentState.getUnitData().getActionInProgressFinishTimeByIndex(index);
        
        DrawActionType(actionInProgress, progress, width);
        double remainingTime = (finishTime - _currentState.getCurrentFrame());
        double timeRatio = remainingTime / actionInProgress.buildTime();
        Position ratioBar((PositionType)(timeRatio * progressBar.x()), progressBar.y());

        GUITools::DrawRect(progress + progressBuffer - Position(2,2), progress + progressBuffer + Position(2,2) + progressBar, grey2);
        GUITools::DrawRect(progress + progressBuffer, progress + progressBuffer + progressBar, grey);
        GUITools::DrawRect(progress + progressBuffer, progress + progressBuffer + ratioBar, blue);

        std::stringstream time; time << (int)remainingTime;
        GUITools::DrawStringLargeWithShadow(progress + progressBuffer + Position(10, 20), time.str(), white);
        progress.add(0, width);
    }

    Position buildings(650, 200);
    const BuildingData & buildingData = _currentState.getBuildingData();
    GUITools::DrawStringLargeWithShadow(buildings, "Completed Buildings", white);
    for (size_t i(0); i < buildingData.size(); ++i)
    {
        const BuildingStatus & buildingStatus = buildingData.getBuilding(i);

        const ActionType & type = buildingStatus._type;
        const FrameCountType & finishTime = buildingStatus._timeRemaining;
        const ActionType & makingType = buildingStatus._isConstructing;

        DrawActionType(type, buildings, width);
        
        if (finishTime > 0)
        {
            double remainingTime = finishTime;
            double timeRatio = remainingTime / makingType.buildTime();
            Position ratioBar((PositionType)(timeRatio * progressBar.x()), progressBar.y());
        
            GUITools::DrawRect(buildings + progressBuffer - Position(2,2), buildings + progressBuffer + Position(2,2) + progressBar, grey2);
            GUITools::DrawRect(buildings + progressBuffer, buildings + progressBuffer + progressBar , grey);
            GUITools::DrawRect(buildings + progressBuffer, buildings + progressBuffer + ratioBar, blue);
            std::stringstream time; time << (int)remainingTime;
            GUITools::DrawStringLargeWithShadow(buildings + progressBuffer + Position(10, 20), time.str(), white);
        }
        
        buildings.add(0, width);
    }

    Position hatcheries(1000, 200);
    const HatcheryData & hatcheryData = _currentState.getHatcheryData();
    GUITools::DrawStringLargeWithShadow(hatcheries, "Hatchery Data", white);
    for (size_t i(0); i < hatcheryData.size(); ++i)
    {
        DrawActionType(Hatchery, hatcheries, width);
        std::stringstream num; num << hatcheryData.getHatchery(i).numLarva();
        for (size_t l(0); l < hatcheryData.getHatchery(i).numLarva(); ++l)
        {
            DrawActionType(Larva, hatcheries + Position(width * (l+1), 0), width);
        }

        //GUITools::DrawStringLargeWithShadow(hatcheries + Position(10, 20), num.str(), white);
        hatcheries.add(0, width);
    }
}

void GUI::DrawActionType(const ActionType & type, const Position & topLeft, const size_t & width)
{
    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    int textureNumber = getTextureNumber(type);
    float ratio = (float)width / textureSizes[textureNumber].x();
    Position size = Position((PositionType)(textureSizes[textureNumber].x() * ratio), (PositionType)(textureSizes[textureNumber].y() * ratio));
    Position bottomRight(topLeft + size);
    GUITools::DrawTexturedRect(topLeft, bottomRight, textureNumber, white);
}

void GUI::SetActionTimes(const std::vector<FrameCountType> & startTimes, std::vector<FrameCountType> & finishTimes)
{
    _startTimes = startTimes;
    _finishTimes = finishTimes;
}

void GUI::SetBuildOrder(const std::vector<ActionType> & buildOrder, const size_t boIndex)
{
    Position bopos(0,0);

    _buildOrder = buildOrder;
    _boIndex = boIndex;
}

void GUI::DrawAllUnits()
{
    GLfloat color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    float width = 64;
    Position p;
    Position techp(p.x(), p.y() + 100);

    for (size_t r(0); r < 3; ++r)
    {
        for (size_t i(0); i < ActionTypes::GetAllActionTypes(r).size(); ++i)
        {   
            const ActionType & type = ActionTypes::GetAllActionTypes(r)[i];
            DrawActionType(type, (type.isUnit() ? p : techp), (PositionType)width);
            
            if (type.isUnit())
            {
                p = p + Position((PositionType)width, 0);
            }
            else
            {
                techp = techp + Position((PositionType)width, 0);
            }
        }
        
        p = Position(0, (r+1) * 200);
        techp = Position(p.x(), p.y() + 100);
    }

}

void GUI::LoadTextures()
{
    textures = std::vector<GLuint>(BOSS_MAX_TEXTURES);
    textureSizes = std::vector<Position>(BOSS_MAX_TEXTURES);
    glGenTextures(BOSS_MAX_TEXTURES,&textures[0]);

    for (size_t r(0); r < Races::NUM_RACES; ++r)
    {
        for (size_t i(0); i < ActionTypes::GetAllActionTypes(r).size(); ++i)
        {
            const ActionType & type = ActionTypes::GetAllActionTypes(r)[i];

            LoadTexture(getTextureNumber(type), getTextureFileName(type).c_str(), false);
        }
    }
}

const int GUI::getTextureNumber(const ActionType & type) const
{
    return BOSS_TEXTURE_INTERVAL * type.getRace() + type.ID();
}

void GUI::LoadTexture(int textureNumber, const char * fileName, bool mipmap)
{
    struct stat buf;
    if (stat(fileName, &buf) == -1)
    {
        return;
    }

    SDL_Surface *surface;
    GLenum texture_format = GL_RGBA;
    GLint  nOfColors;

    if ( (surface = IMG_Load(fileName)) ) 
    { 
        // get the number of channels in the SDL surface
        nOfColors = surface->format->BytesPerPixel;
        if (nOfColors == 4)     // contains an alpha channel
        {
            //printf("Contains Alpha\n");
            if (surface->format->Rmask == 0x000000ff)
            {
                texture_format = GL_RGBA;
            }
        //else
        // texture_format = GL_BGRA;
        } 
        else if (nOfColors == 3)     // no alpha channel
        {
            if (surface->format->Rmask == 0x000000ff)
            {
                texture_format = GL_RGB;
            }
            //else
            // texture_format = GL_BGR;
        } 
        else 
        {
            printf("warning: %s is not truecolor..  this will probably break\n", fileName);
            // this error should not go unhandled
        }

        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, textureNumber );

		if (mipmap)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, surface->w, surface->h, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, nOfColors, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);
		}
    } 
    else 
    {
        printf("SDL could not load image: %s\n", SDL_GetError());
    }    

    // Free the SDL_Surface only if it was successfully created
    if ( surface ) 
    { 
        textureSizes[textureNumber] = Position(surface->w, surface->h);

        //printf("Loaded Image %s\n", fileName);
        SDL_FreeSurface( surface );
    }
}

void GUI::SetState(const GameState & state)
{
    _currentState = state;
}

bool GUI::saveScreenshotBMP(const std::string & filename) 
{
    SDL_Surface * image = SDL_CreateRGBSurface(SDL_SWSURFACE, windowSizeX, windowSizeY, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, windowSizeX, windowSizeY, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);

    SDL_Surface * imageFlipped = GUITools::flip_surface(image, GUITools::FLIP_VERTICAL);

    SDL_SaveBMP(imageFlipped, filename.c_str());
    SDL_FreeSurface(image);
    SDL_FreeSurface(imageFlipped);

    return true;
}

const std::string GUI::getTextureFileName(const ActionType & type) const
{
	std::string filename = "../starcraft_images/units/" + type.getName() + ".png";

    if (type.isTech())
    {
        filename = "../starcraft_images/command_icons/" + type.getName() + ".png";
    }
    else if (type.isUpgrade())
    {
        filename = "../starcraft_images/command_icons/" + type.getName() + ".png";
    }

	for (size_t i(0); i<filename.size(); ++i)
	{
		if (filename[i] == ' ')
		{
			filename[i] = '_';
		}
	}

	return filename;
}

#ifdef WIN32
    // disable vsync in windows
    #include "glext/wglext.h"
    #include "glext/glext.h"
    typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );
    PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;

void GUI::setVSync(int interval)
{
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress( "wglSwapIntervalEXT" );

    if( wglSwapIntervalEXT )
    {
        if(wglSwapIntervalEXT(interval))
        {
            //puts("VSync changed");
        } 
        else 
        {
            //puts("VSync change failed");
        }
    }
    else 
    {
        //puts("VSync change unsupported");
    } 
}

#endif

#endif