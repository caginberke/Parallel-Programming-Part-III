#include "icb_gui.h"


enum HitPos
{
    None,
    Left,
    Middle,
    Right,
    End
};


struct MyStruct
{
    HWND main;              //Main window handle

    int shipX, shipY;       //Ship coordinates
    bool boxActive;         //Ýf the box active
    HitPos boxHit;          //Current hit position
    int boxX, boxY;         //Box coordinates
    int FRM1;               //Frame
    int bulletX, bulletY;   //Bullet coordinates

    bool bulletActive = false;


    //int width, height, posX, posY;
    void Restart()
    {
        shipY = 590;
        shipX = 280;
        boxActive = true;
        boxHit = None;
        boxX = rand() % 541;
        boxY = 0;
    }
};


void ICGUI_Create()
{
    ICG_MWTitle("Shooting Game");
    ICG_MWSize(700, 700);
    ICG_MWPosition(0, 0);
    //ICG_MW_RemoveTitleBar();
}





int keypressed = 0;
ICBYTES map;
HANDLE HMutex = CreateMutex(NULL, FALSE, NULL);


void* ShipThread(PVOID lpParam)
{
    MyStruct* data = (MyStruct*)lpParam;

    while (TRUE)
    {

        //Draw the ship
        FillRect(map, data->shipX, data->shipY, 20, 6, 0xffff00);
        DisplayImage(data->FRM1, map);
        Sleep(10);
        FillRect(map, data->shipX, data->shipY, 20, 6, 0);


        //Move the ship
        if (keypressed == 39 && data->shipX < 540)
            data->shipX += 3;

        else if (keypressed == 37 && data->shipX > 0)
            data->shipX -= 3;

    }
    return NULL;
}

void* BoxThread(PVOID lpParam)
{
    MyStruct* data = (MyStruct*)lpParam;

    //Update box based on current hit status

    while (TRUE)
    {
        switch (data->boxHit)
        {
        case None:
            data->boxY += 2;
            if (data->boxY > 620)
                data->boxHit = End;
            break;

        case Left:
            if ((data->boxY -= 5) <= 1 || (data->boxX -= 2) <= 1)
                data->boxHit = End;
            break;

        case Middle:
            if ((data->boxY -= 5) <= 1)
                data->boxHit = End;
            break;

        case Right:
            if ((data->boxY -= 5) <= 1 || (data->boxX += 2) >= 560)
                data->boxHit = End;
            break;


        case End:
            data->boxHit = None;
            data->boxX = rand() % 540;
            data->boxY = 0;
            break;

        }

        //Draw the box
        FillRect(map, data->boxX, data->boxY, 20, 20, 0xffff00);
        DisplayImage(data->FRM1, map);
        Sleep(10);
        FillRect(map, data->boxX, data->boxY, 20, 20, 0);
    }


    return NULL;
}

void* BulletThread(PVOID lpParam)
{

    MyStruct* data = (MyStruct*)lpParam;

    DWORD dwWaitResult;

    bool mutex = false;

    while (TRUE)
    {
        //If bullet is not active, wait and continue
        if (!data->bulletActive)
        {
            Sleep(30);
            continue;
        }

        //Wait for mutex
        if (!mutex)
        {
            mutex = true;
            dwWaitResult = WaitForSingleObject(HMutex, INFINITE);
        }

        //Draw and move the bullet
        FillRect(map, data->bulletX, data->bulletY, 3, 10, 0xFF0000);
        DisplayImage(data->FRM1, map);
        Sleep(20);
        FillRect(map, data->bulletX, data->bulletY, 3, 10, 0);

        data->bulletY -= 15;

        //Check if the bullet in the screen  
        if (data->bulletY < 0)
        {
            data->bulletActive = false;
            if (mutex)
            {
                mutex = false;
                ReleaseMutex(HMutex);
            }
        }

        //Check if the bullet hit 
        if (data->bulletY - 20 < data->boxY && data->boxHit == None)
        {
            if (data->bulletX >= data->boxX && data->bulletX <= data->boxX + 5)
            {
                data->boxHit = Right;
            }
            if (data->bulletX >= data->boxX + 6 && data->bulletX <= data->boxX + 13)
            {
                data->boxHit = Middle;
            }
            if (data->bulletX >= data->boxX + 14 && data->bulletX <= data->boxX + 20)
            {
                data->boxHit = Left;
            }

        }
    }
}


void* BulletChecker(PVOID lpParam)
{
    MyStruct* data = (MyStruct*)lpParam;

    DWORD dwWaitResult;

    while (true)
    {
        //Fire the bullet
        if (keypressed == 32)
        {
            keypressed = 0;

            //Wait 200 ms for mutex
            dwWaitResult = WaitForSingleObject(HMutex, 200);

            //If mutex is available
            if (dwWaitResult == WAIT_OBJECT_0)
            {
                //Fire the bullet
                data->bulletActive = true;
                data->bulletX = data->shipX + 9;
                data->bulletY = 580;

                //Release the mutex
                ReleaseMutex(HMutex);
            }

        }
    }
}



void Start(void* arg);
void WhenKeyPressed(int k) { keypressed = k; }

void ICGUI_main()

{
    //Creating Struct and Main 
    MyStruct* gameData = new MyStruct;

    gameData->main = ICG_GetMainWindow();
    gameData->FRM1 = ICG_FrameMedium(5, 5, 560, 620);
    ICG_Button(570, 5, 100, 25, "BAÞLAT", Start, gameData);
    ICG_SetOnKeyPressed(WhenKeyPressed);
}

void Start(void* arg)
{

    MyStruct* data = (MyStruct*)arg;

    SetFocus(ICG_GetMainWindow());

    //Init Game Data
    data->Restart();

    //Init Game Screen
    CreateImage(map, 560, 620, ICB_UINT);

    DWORD dwWaitResult;

    //All Threads
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShipThread, arg, 0, 0);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BulletThread, arg, 0, 0);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BoxThread, arg, 0, 0);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BulletChecker, arg, 0, 0);



}