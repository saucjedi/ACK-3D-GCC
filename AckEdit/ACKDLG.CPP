// Options dialog module
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ackres.h"
#include "ackedit.h"

#include "ack3d.h"
extern  unsigned    short   *ObjGrid;

extern  WORD    nWallFlags;
extern  WORD    nObjFLags;
extern  short   CurrentBitmap;
extern  short   nEditType;
extern  header  BufferHeader;
extern  ACKENG  *ae;
extern  HPALETTE  hpalApp;
extern  BOOL    bModified;

short SaveTempInfo(HWND hWnd);
void SetObjectEdits(HWND hDlg);
void GetObjectEdit(HWND hDlg);
void AddFileName(short nType,char *szName);
void RemoveFileName(short nType,short nIndex);
BOOL CALLBACK _export FAR PASCAL OptionsDlgProc(HWND hDlg, UINT iMessage,
        WPARAM wParam, LPARAM lParam);
BOOL CALLBACK _export FAR PASCAL InfoDlgProc(HWND hDlg, UINT iMessage,
        WPARAM wParam, LPARAM lParam);

    void    *pSupplyBuffer;
    HDC     SupplyBuffer;
    short   nSupplyTop;
    short   nSupplySelected;
    short   nCurrentTop;
    short   nCurrentType;
    HBITMAP hOldBitmap;
    UCHAR   bmCurrent[256];
    NEWOBJECT   *pObj;
    OBJSEQ      *pCurrentOS;

    short   nObjIDList[] = {
            IDC_CREATE,IDC_WALK,IDC_INTERACT,IDC_ATTACK,IDC_DESTROY,
            IDC_OBJPASS,IDC_ACTIVE,IDC_MOVEABLE,IDC_VSCROLLSUPPLY,
            IDC_VSCROLLCURRENT,IDC_MULTIVIEW,IDC_SIDES,IDC_BMPERVIEW,
            IDC_ANIMONCE,IDC_SIDESSTATIC,IDC_BPVSTATIC,IDC_ANIMATE,
            IDC_SPEEDSTATIC,IDC_SPEED,IDC_LOAD,0
            };

    short   nWallIDList[] = {
            IDC_NORMAL,IDC_DOORSPLIT,IDC_DOORSLIDING,IDC_DOORSECRET,
            IDC_TRANSPARENT,IDC_WALLPASS,0
            };

//MEH
extern unsigned char GenericUseBitmapBuffer[4*64*64];   //Declared in ACKEDIT.CPP
unsigned char GenericUseBitmapBuffer2[64*64];
extern   PalletteSlot pallette[256];

//Use to let mouse wheel scroll current/supply bmps
#define     LAST_USED_CURRENT   0
#define     LAST_USED_SUPPLY    1
unsigned char cLastScrollboxUsed = LAST_USED_SUPPLY;

extern bool g_bCreatedObjectThisCycle;

extern MainWindow* MainAppWnd;

void DeleteBitmap(HWND hWnd,HWND hwndChild);
// --MEH

//MEH Copy a NEWOBJECT into another NEWOBJECT
void CopyObject(NEWOBJECT* nObjSrc, NEWOBJECT* nObjDest)
{
    if(nObjSrc == NULL || nObjDest == NULL)
        return;

    //Shallow copy relevant data
    memcpy(nObjDest, nObjSrc, sizeof(NEWOBJECT));

    //Now deep copy object sequence bitmaps
    for(int i = 0; i < MAX_OBJ_BITMAPS; i++)
    {
        nObjDest->Create.bitmaps[i] = nObjSrc->Create.bitmaps[i];
        nObjDest->Destroy.bitmaps[i] = nObjSrc->Destroy.bitmaps[i];
        nObjDest->Walk.bitmaps[i] = nObjSrc->Walk.bitmaps[i];
        nObjDest->Attack.bitmaps[i] = nObjSrc->Attack.bitmaps[i];
        nObjDest->Interact.bitmaps[i] = nObjSrc->Interact.bitmaps[i];
    }

    //Set to point at right bitmaps for the current object type
    switch(nObjSrc->CurrentType)
    {
        case NO_CREATE:
            nObjDest->CurrentBitmaps = (nObjDest->Create.bitmaps);
            break;
        case NO_DESTROY:
            nObjDest->CurrentBitmaps = (nObjDest->Destroy.bitmaps);
            break;
        case NO_WALK:
            nObjDest->CurrentBitmaps = (nObjDest->Walk.bitmaps);
            break;
        case NO_ATTACK:
            nObjDest->CurrentBitmaps = (nObjDest->Attack.bitmaps);
            break;
        case NO_INTERACT:
            nObjDest->CurrentBitmaps = (nObjDest->Interact.bitmaps);
            break;
    }
}

//**************************************************************************
//
//**************************************************************************
int DoOptionsDlg(HINSTANCE hInst, HWND hWnd)
{
    DLGPROC   lpfnOptionsDlgProc;
    int       iReturn;
    NEWOBJECT oOldObj;  //Used to hold the data for when the object dialog is opened, in case user hits cancel, so we can copy
                        //the old data back over.
    WORD      nOldWallFlags;    //Used to hold onto old wall flags in case user presses cancel, kinda like we do with objects

    if(nEditType == EDIT_OBJECTS && !g_bCreatedObjectThisCycle)
        CopyObject(ae->ObjList[CurrentBitmap], &oOldObj);
    if(nEditType == EDIT_WALLS)
        nOldWallFlags = nWallFlags; //Hold onto our old flags

    lpfnOptionsDlgProc = (DLGPROC)MakeProcInstance((FARPROC)OptionsDlgProc, hInst);

    iReturn = DialogBox(hInst,MAKEINTRESOURCE(DIALOG_1),hWnd,lpfnOptionsDlgProc);

    FreeProcInstance((FARPROC)lpfnOptionsDlgProc);

    if (iReturn < 0)
        MessageBox(hWnd,"Unable to open dialog box","Error",MB_OK);
    else if(!iReturn && nEditType == EDIT_OBJECTS)   //Returned false, which means user clicked Cancel
    {
        //Restore old object
        if(!g_bCreatedObjectThisCycle)
            CopyObject(&oOldObj, ae->ObjList[CurrentBitmap]);
        else    //Destroy newly created object
        {
            DeleteBitmap(MainAppWnd->GetHandle(),MainAppWnd->GetChildHandle());
        }
    }
    else if(!iReturn && nEditType == EDIT_WALLS)
        nWallFlags = nOldWallFlags; //Restore old wall flags if user hit cancel


    return(iReturn);
}

//**************************************************************************
//
//**************************************************************************
int DoInfoDlg(HINSTANCE hInst, HWND hWnd)
{
    DLGPROC   lpfnInfoDlgProc;
    int       iReturn;

if (SaveTempInfo(hWnd))
    return(-1);

lpfnInfoDlgProc = (DLGPROC)MakeProcInstance((FARPROC)InfoDlgProc, hInst);

iReturn = DialogBox(hInst,MAKEINTRESOURCE(DIALOG_3),hWnd,lpfnInfoDlgProc);

FreeProcInstance((FARPROC)lpfnInfoDlgProc);

if (iReturn < 0)
    MessageBox(hWnd,"Unable to open dialog box","Error",MB_OK);


return(iReturn);
}


//**************************************************************************
//
//**************************************************************************
void SetupWinG(HWND hDlg)
{
    RECT    rect;
    HBITMAP hbm;

SupplyBuffer = CreateCompatibleDC(GetDC(hDlg));
hbm = CreateCompatibleBitmap(GetDC(hDlg),64,64);

hOldBitmap = (HBITMAP)SelectObject(SupplyBuffer, hbm);

PatBlt(SupplyBuffer, 0,0,rect.right,rect.bottom, BLACKNESS);

}

//**************************************************************************
//
//**************************************************************************
void DisableControls(HWND hDlg,short *pIDList)
{
    short   i;

i = 0;
while (pIDList[i])
    EnableWindow(GetDlgItem(hDlg,pIDList[i++]),FALSE);

}


//**************************************************************************
//
//**************************************************************************
void ShowCurrent(HWND hDlg)
{
    RECT        rect;
    short       y,y1,nIndex,OrgIndex,bx,by;
    UCHAR       *pPtr,*pBuf;
    HWND        hWnd;
    HDC         hdc;
    char        szString[80];

hWnd = GetDlgItem(hDlg,IDC_CURRENT);
hdc = GetDC(hWnd);
memset(GenericUseBitmapBuffer2, 0, 64*64);//Init bmp data to 0

if (hpalApp)
    {
    UnrealizeObject(hpalApp);
    SelectPalette(hdc, hpalApp, FALSE);
    RealizePalette(hdc);
    }

GetClientRect(hWnd,&rect);
PatBlt(hdc,0,0,rect.right,rect.bottom,WHITENESS);   //MEH draw it white

OrgIndex = nIndex = nCurrentTop;
y1 = rect.bottom - 64;
y = 2;
while (y < y1)
    {
    pPtr = ae->oMaps[bmCurrent[nIndex++]];

    if (pPtr == NULL)
        {
        Rectangle(hdc,2,y,61,y+64);
        }
    else
        {
        for (bx = 0; bx < 64; bx++)
            {
            pBuf = (UCHAR *)GenericUseBitmapBuffer2 + bx;
            for (by = 0; by < 64; by++)
                {
                *pBuf = *pPtr++;
                pBuf += 64;
                }
            }

        //MEH Draw stuffz!
        unsigned char* pBufferData = GenericUseBitmapBuffer;
        for(int r = 0; r < 64; r++)
        {
            for(int c = 0; c < 64; c++) //AHAHA JOKE
            {
                int red = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].r;
                int green = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].g;
                int blue = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].b;
                if(red == 0 && green == 0 && blue == 0)
                    *pBufferData++ = 0; //Alpha = 0 if this pixel is black
                else
                    *pBufferData++ = 255; //Alpha = 255 by default.
            }
        }
        HICON ic = CreateIcon(Main::hInstance, 64, 64, 1, 32, NULL, (BYTE*)(GenericUseBitmapBuffer));
        DrawIconEx(hdc, 2, y, ic, 64-5, 64, NULL, NULL, DI_NORMAL); //Stretch the bitmap slightly so it fits
        DestroyIcon(ic);

        }

    if (nIndex > MAX_OBJ_BITMAPS)
        nIndex = 0;

    if (nIndex == OrgIndex)
        break;

    y += 67;
    }

ReleaseDC(hWnd,hdc);
wsprintf(szString,"Current:%d",strlen((const char*)bmCurrent));
SetWindowText(GetDlgItem(hDlg,IDC_CURSTATIC),szString);

}



//**************************************************************************
//
//**************************************************************************
void ShowSupply(HWND hDlg)
{
    RECT        rect;
    short       y,y1,nIndex,OrgIndex,bx,by;
    UCHAR       *pPtr,*pBuf;
    HWND        hWnd;
    HDC         hdc;

hWnd = GetDlgItem(hDlg,IDC_SUPPLY);
hdc = GetDC(hWnd);
memset(GenericUseBitmapBuffer2, 0, 64*64);//Init bmp data to 0
if (hpalApp)
    {
    UnrealizeObject(hpalApp);
    SelectPalette(hdc, hpalApp, FALSE);
    RealizePalette(hdc);
    }

GetClientRect(hWnd,&rect);
PatBlt(hdc,0,0,rect.right,rect.bottom,WHITENESS);   //MEH draw it white

OrgIndex = nIndex = nSupplyTop;
y1 = rect.bottom - 64;
y = 2;
while (y < y1)
    {
    pPtr = ae->oMaps[nIndex++];

    if (pPtr == NULL)
        {
        Rectangle(hdc,2,y,61,y+64);
        }
    else
        {
        for (bx = 0; bx < 64; bx++)
            {
            pBuf = (UCHAR *)GenericUseBitmapBuffer2 + bx;
            for (by = 0; by < 64; by++)
                {
                *pBuf = *pPtr++;
                pBuf += 64;
                }
            }


        //MEH Draw stuffz!
        unsigned char* pBufferData = GenericUseBitmapBuffer;
        for(int r = 0; r < 64; r++)
        {
            for(int c = 0; c < 64; c++) //AHAHA JOKE
            {
                int red = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].r;
                int green = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].g;
                int blue = *pBufferData++ = pallette[GenericUseBitmapBuffer2[c+64*r]].b;
                if(red == 0 && green == 0 && blue == 0)
                    *pBufferData++ = 0; //Alpha = 0 if this pixel is black
                else
                    *pBufferData++ = (nEditType == EDIT_OBJECTS)?(255):(128); //Alpha = 255 by default. If this is the wall options
                                                                              // dialog, though, make images greyed out.
            }
        }
        HICON ic = CreateIcon(Main::hInstance, 64, 64, 1, 32, NULL, (BYTE*)(GenericUseBitmapBuffer));
        DrawIconEx(hdc, 2, y, ic, 64-5, 64, NULL, NULL, DI_NORMAL); //Squish the bitmap a tad so it fits
        DestroyIcon(ic);

        }

    if (nIndex > 255)
        nIndex = 1;

    if (nIndex == OrgIndex)
        break;

    y += 67;
    }

ReleaseDC(hWnd,hdc);
}


//**************************************************************************
//
//**************************************************************************
void CloseWinG(void)
{
    HBITMAP hbm;

if (SupplyBuffer)
    {
    hbm = (HBITMAP)SelectObject(SupplyBuffer,hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(SupplyBuffer);
    }

}

//**************************************************************************
//
//**************************************************************************
void DoScroll(short nCode,short nSBPos,HWND hDlg,HWND hWndScroll)
{
    short   nPos;
    BOOL    bSupply = TRUE;
    BOOL    bShow = FALSE;


if (hWndScroll == GetDlgItem(hDlg,IDC_VSCROLLSUPPLY))
{
    nPos = nSupplyTop;
    cLastScrollboxUsed = LAST_USED_SUPPLY;
}
else
{
    nPos = nCurrentTop;
    bSupply = FALSE;
    cLastScrollboxUsed = LAST_USED_CURRENT;
}

switch (nCode)
    {
    case SB_LINEUP:
        if (nPos)
            {
            nPos--;
            bShow = TRUE;
            }
        break;
    case SB_LINEDOWN:
        if(bSupply && nPos < 252)
        {
            nPos++;
            bShow = TRUE;
        }
        else if((!bSupply) && nPos < 29)
        {
            nPos++;
            bShow = TRUE;
        }
        break;
    case SB_PAGEUP:
        nPos -= 3;
        if (nPos < 0)
            nPos = 0;
        bShow = TRUE;
        break;

    case SB_PAGEDOWN:
        nPos += 3;
        if(bSupply && nPos > 252)
            nPos = 252;
        else if((!bSupply) && nPos > 29)
            nPos = 29;
        bShow = TRUE;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nPos = nSBPos;
        if (nPos < 0)
            nPos = 0;
        if (bSupply && nPos > 252)
            nPos = 252;
        else if((!bSupply) && nPos > 29)
            nPos = 29;
        bShow = TRUE;
        break;

    default:
        break;
    }

if (bShow)
    {
    SetScrollPos(hWndScroll,SB_CTL,nPos,TRUE);
    if (bSupply)
        {
        if (!nPos)
            nPos = 1;
        nSupplyTop = nPos;
        ShowSupply(hDlg);
        }
    else
        {
        nCurrentTop = nPos;
        ShowCurrent(hDlg);
        }
    }
}

//**************************************************************************
//
//**************************************************************************
void TransferCurrentToSupply(HWND hDlg,short mx,short my)
{
    short   nBitmap,y;
    RECT    rect;

    if(nEditType == EDIT_WALLS)
        return;     //MEH Shouldn't be able to transfer if we're not even editing objects!

GetWindowRect(GetDlgItem(hDlg,IDC_CURRENT),&rect);
y = my - rect.top;
nBitmap = nCurrentTop + (y / 66);
if (bmCurrent[nBitmap] == 0)
    return;

for (y = nBitmap; y < 254; y++)
    bmCurrent[y] = bmCurrent[y+1];

//MEH Now update the bitmaps per view and such...
GetObjectEdit(hDlg);
SetObjectEdits(hDlg);

ShowCurrent(hDlg);
}



//**************************************************************************
//
//**************************************************************************
void TransferSupplyToCurrent(HWND hDlg,short mx,short my)
{
    short   nBitmap,y;
    RECT    rect;

    if(nEditType == EDIT_WALLS)
        return;     //MEH Shouldn't be able to transfer if we're not even editing objects!

GetWindowRect(GetDlgItem(hDlg,IDC_SUPPLY),&rect);
y = my - rect.top;
nBitmap = nSupplyTop + (y / 66);

if (ae->oMaps[nBitmap] == NULL)
    return;

BOOL bFoundOne = 0;
for (y = 0; y < MAX_OBJ_BITMAPS; y++)
    {
    if (!bmCurrent[y] && !bFoundOne)
        {
        bmCurrent[y] = nBitmap;
        ShowCurrent(hDlg);
        GetObjectEdit(hDlg);
        SetObjectEdits(hDlg);
        bFoundOne = 1;
        continue;
        }
    //MEH Brute-force these random blank bitmaps out
    if(bFoundOne)
        bmCurrent[y] = 0;
    }
}

//**************************************************************************
//
//**************************************************************************
void DeleteSupplyBitmap(HWND hDlg,short mx,short my)
{
    short   nBitmap,y;
    RECT    rect;
    HWND    hDlgItem;

hDlgItem = GetDlgItem(hDlg,IDC_SUPPLY);
GetWindowRect(hDlgItem,&rect);
y = my - rect.top;
nBitmap = nSupplyTop + (y / 66);

if (ae->oMaps[nBitmap] == NULL)
    return;


if (MessageBox(hDlg,"Delete this bitmap?","Delete",MB_YESNO) == IDYES)
    {
    AckFree(ae->oMaps[nBitmap]);
    ae->oMaps[nBitmap] = NULL;
    RemoveFileName(TYPE_OBJECT,nBitmap);
    ShowSupply(hDlg);
    SetUndoPoint();
    }

}


//**************************************************************************
//
//**************************************************************************
void RestoreBitmaps(void)
{

if (pCurrentOS == NULL)
    return;

memset(pCurrentOS->bitmaps, 0, MAX_OBJ_BITMAPS);
memcpy(pCurrentOS->bitmaps,bmCurrent,strlen((const char*)bmCurrent));
pCurrentOS->MaxBitmaps = strlen((const char*)bmCurrent);

if (pCurrentOS->bmSides)
    pCurrentOS->MaxBitmaps /= pCurrentOS->bmSides;
else
    pCurrentOS->bmSides = 1;
}


//**************************************************************************
//
//**************************************************************************
void MoveBitmaps(HWND hDlg,OBJSEQ *pOS)
{

RestoreBitmaps();
pCurrentOS = pOS;
memset(bmCurrent, 0, MAX_OBJ_BITMAPS);
memcpy(bmCurrent,pOS->bitmaps,strlen((const char*)pOS->bitmaps));
nCurrentTop = 0;
InvalidateRect(hDlg,NULL,FALSE);
ShowCurrent(hDlg);
}

//**************************************************************************
//
//**************************************************************************
void SetWallControls(HWND hDlg)
{
    short   nID;

nID = IDC_NORMAL;
if (nWallFlags & DOOR_TYPE_SLIDE)
    nID = IDC_DOORSLIDING;
if (nWallFlags & DOOR_TYPE_SPLIT)
    nID = IDC_DOORSPLIT;
if (nWallFlags & DOOR_TYPE_SECRET)
    nID = IDC_DOORSECRET;

SendDlgItemMessage(hDlg,nID,BM_SETCHECK,TRUE,0L);
nID = FALSE;
if (nWallFlags & WALL_TYPE_TRANS)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_TRANSPARENT,BM_SETCHECK,nID,0L);

nID = FALSE;
if (nWallFlags & WALL_TYPE_PASS)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_WALLPASS,BM_SETCHECK,nID,0L);
}

//**************************************************************************
//
//**************************************************************************
void SetObjectControls(HWND hDlg)
{
    short   nID;
    UCHAR   nFlags;

if (pCurrentOS == NULL)
    return;

nFlags = pCurrentOS->flags;

nID = FALSE;
if (nFlags & OF_PASSABLE)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_OBJPASS,BM_SETCHECK,nID,0L);

nID = FALSE;
if (nFlags & OF_MOVEABLE)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_MOVEABLE,BM_SETCHECK,nID,0L);

nID = FALSE;
if (nFlags & OF_MULTIVIEW)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_MULTIVIEW,BM_SETCHECK,nID,0L);

nID = FALSE;
if (nFlags & OF_ANIMONCE)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_ANIMONCE,BM_SETCHECK,nID,0L);

nID = FALSE;
if (nFlags & OF_ANIMATE)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_ANIMATE,BM_SETCHECK,nID,0L);

nID = FALSE;
if (pObj->Active || pObj->CurrentBitmaps == NULL)
    nID = TRUE;
SendDlgItemMessage(hDlg,IDC_ACTIVE,BM_SETCHECK,nID,0L);

}

//**************************************************************************
//
//**************************************************************************
void SetObjectEdits(HWND hDlg)
{
    char    szString[20];

if (pCurrentOS == NULL)
    return;

wsprintf(szString,"%d",pCurrentOS->bmSides);
SetWindowText(GetDlgItem(hDlg,IDC_SIDES),szString);
wsprintf(szString,"%d",pCurrentOS->bmBitmapsPerView);
SetWindowText(GetDlgItem(hDlg,IDC_BMPERVIEW),szString);
wsprintf(szString,"%d",pObj->Speed);
SetWindowText(GetDlgItem(hDlg,IDC_SPEED),szString);
}

//**************************************************************************
//
//**************************************************************************
void GetObjectEdit(HWND hDlg)
{
    USHORT  nCode;
    char    szString[40];

if (pCurrentOS == NULL)
    return;

GetWindowText(GetDlgItem(hDlg,IDC_SIDES),szString,sizeof(szString));
pCurrentOS->bmSides = atoi(szString);
GetWindowText(GetDlgItem(hDlg,IDC_BMPERVIEW),szString,sizeof(szString));
pCurrentOS->bmBitmapsPerView = atoi(szString);
GetWindowText(GetDlgItem(hDlg,IDC_SPEED),szString,sizeof(szString));
pObj->Speed = atoi(szString);

RestoreBitmaps();

if (!pObj->CurrentType)
    pObj->CurrentType = NO_CREATE;

if (!pCurrentOS->bmSides)
    pCurrentOS->bmSides = 1;

if (!pCurrentOS->bmBitmapsPerView)
    pCurrentOS->bmBitmapsPerView = 1;

if (pCurrentOS->bmSides == 1)
    pCurrentOS->bmBitmapsPerView = strlen((const char*)bmCurrent);

nCode = ObjGrid[pObj->mPos];
AckSetupObject(ae,CurrentBitmap,nCurrentType,pCurrentOS);
ObjGrid[pObj->mPos] = nCode;
}

//**************************************************************************
//
//**************************************************************************
void SetObjectData(HWND hDlg)
{
SetObjectControls(hDlg);
SetObjectEdits(hDlg);
}

//**************************************************************************
//
//**************************************************************************
void CheckObject(short nType)
{
    USHORT  nCode;

nCode = ObjGrid[pObj->mPos];
if (pObj->CurrentType == nType)
    AckSetObjectType(ae,CurrentBitmap,nType);

if (pObj->CurrentBitmaps == NULL)
   AckSetObjectType(ae,CurrentBitmap,NO_CREATE);
ObjGrid[pObj->mPos] = nCode;
}

//**************************************************************************
//
//**************************************************************************
void SetNewObjectSequence(HWND hDlg,OBJSEQ *pOS,short nType)
{
GetObjectEdit(hDlg);
MoveBitmaps(hDlg,pOS);
SetObjectData(hDlg);
nCurrentType = nType;
CheckObject(nType);
}

//**************************************************************************
//
//**************************************************************************
extern short ErrorCode;
void LoadNewSupply(HWND hWnd)
{
    short   i,temp;
    char    szName[128];

for (i = 1; i < MAX_OBJBMPS; i++)
    {
    if (ae->oMaps[i] == NULL)
        break;
    }

if (i > 254)
    {
    MessageBox(hWnd,"No more empty bitmap slots","ERROR",MB_OK);
    return;
    }

*szName = '\0';
if (!ACKFileOpen(hWnd,szName,sizeof(szName),
     "GIF Images (*.gif)\0*.gif\0PCX files (*.pcx)\0*.pcx\0Object Bitmaps (*.bbm)\0*.bbm\0All Files (*.*)\0*.*\0",
     "Open Bitmap File","bbm"))
    {
    AckCloseResource();
    if (strstr(szName,".pcx") || strstr(szName,".PCX"))
        ae->bmLoadType = BMLOAD_PCX;
    else if (strstr(szName,".gif") || strstr(szName,".GIF"))
        ae->bmLoadType = BMLOAD_GIF;

    //MEH Test result rather than blindly assuming it's fine
    int result = AckLoadBitmap(ae,i,TYPE_OBJECT,szName);
    if(result)
    {
        sprintf(szName, "Error %d in loading bitmap %d . Errorcode: %d", result, i, ErrorCode);
        MessageBox(hWnd, szName, "Error", MB_OK);
    }

    ae->bmLoadType = BMLOAD_BBM;
    temp = CurrentBitmap;
    CurrentBitmap = i;
    if(!result)
        AddFileName(TYPE_OBJECT,szName);
    CurrentBitmap = temp;
    InvalidateRect(hWnd,NULL,FALSE);
    }

}

//**************************************************************************
//
//**************************************************************************
BOOL CALLBACK _export FAR PASCAL OptionsDlgProc(HWND hDlg, UINT iMessage,
        WPARAM wParam, LPARAM lParam)
{
    short   mx,my;
    RECT    rect;
    POINT   pt;
         HWND           hDlgItem;

switch (iMessage)
    {
    case WM_INITDIALOG:
        memset(bmCurrent,0,256);
        hDlgItem = GetDlgItem(hDlg,IDC_CURRENT);
        GetClientRect(hDlgItem,&rect);
        if (rect.right < 64)
            SetWindowPos(hDlgItem,NULL,0,0,64,rect.bottom,
                         SWP_NOZORDER|SWP_NOMOVE|SWP_NOREDRAW);

        hDlgItem = GetDlgItem(hDlg,IDC_SUPPLY);
        GetClientRect(hDlgItem,&rect);
        if (rect.right < 64)
            SetWindowPos(hDlgItem,NULL,0,0,64,rect.bottom,
                         SWP_NOZORDER|SWP_NOMOVE|SWP_NOREDRAW);

        if (nEditType != EDIT_OBJECTS)
            {
            DisableControls(hDlg,&nObjIDList[0]);
            SetWallControls(hDlg);
            }
        else
            {
            DisableControls(hDlg,&nWallIDList[0]);
            SendDlgItemMessage(hDlg,IDC_CREATE,BM_SETCHECK,TRUE,0L);
            wsprintf((char*)bmCurrent,"Object: %d",CurrentBitmap);
            SetWindowText(GetDlgItem(hDlg,IDC_GROUPBOX2),(const char*)bmCurrent);
            pObj = ae->ObjList[CurrentBitmap];
            nCurrentType = NO_CREATE;
            memset(bmCurrent,0,MAX_OBJ_BITMAPS);
            pCurrentOS = &pObj->Create;
            memcpy(bmCurrent,pCurrentOS->bitmaps,strlen((const char*)pCurrentOS->bitmaps));
            SetObjectControls(hDlg);
            SetObjectEdits(hDlg);
            }

        SupplyBuffer = 0;
        SetupWinG(hDlg);
        nSupplyTop = 1;
        nCurrentTop = 0;
        ShowSupply(hDlg);
        ShowCurrent(hDlg);
        SetScrollRange(GetDlgItem(hDlg,IDC_VSCROLLSUPPLY),SB_CTL,0,252,FALSE);
        SetScrollRange(GetDlgItem(hDlg,IDC_VSCROLLCURRENT),SB_CTL,0,MAX_OBJ_BITMAPS-3,FALSE);
        SetScrollPos(GetDlgItem(hDlg,IDC_VSCROLLSUPPLY),SB_CTL,0,FALSE);
        SetScrollPos(GetDlgItem(hDlg,IDC_VSCROLLCURRENT),SB_CTL,0,FALSE);
        cLastScrollboxUsed = LAST_USED_SUPPLY;
        return(TRUE);

    case WM_LBUTTONUP:    //MEH Left-clicking, rather than double-clicking, moves bitmaps. I HATE double-clicking.
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ClientToScreen(hDlg,&pt);
        mx = pt.x;
        my = pt.y;
        GetWindowRect(GetDlgItem(hDlg,IDC_SUPPLY),&rect);
        if (mx >= rect.left && mx <= rect.right &&
            my >= rect.top && my <= rect.bottom)
            {
            TransferSupplyToCurrent(hDlg,mx,my);
            cLastScrollboxUsed = LAST_USED_SUPPLY;
            }
        else
            {
            GetWindowRect(GetDlgItem(hDlg,IDC_CURRENT),&rect);
            if (mx >= rect.left && mx <= rect.right &&
                my >= rect.top && my <= rect.bottom)
                {
                TransferCurrentToSupply(hDlg,mx,my);
                cLastScrollboxUsed = LAST_USED_CURRENT;
                }
            }
        break;

    case WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ClientToScreen(hDlg,&pt);
        mx = pt.x;
        my = pt.y;
        GetWindowRect(GetDlgItem(hDlg,IDC_SUPPLY),&rect);
        if (mx >= rect.left && mx <= rect.right &&
            my >= rect.top && my <= rect.bottom)
        {
            if(nEditType == EDIT_OBJECTS)
                DeleteSupplyBitmap(hDlg,mx,my);
            cLastScrollboxUsed = LAST_USED_SUPPLY;
        }
        break;

    //MEH Right-clicking current bitmaps also removes them. Just for fun.
    case WM_RBUTTONUP:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ClientToScreen(hDlg,&pt);
        mx = pt.x;
        my = pt.y;
        GetWindowRect(GetDlgItem(hDlg,IDC_CURRENT),&rect);
        if (mx >= rect.left && mx <= rect.right &&
            my >= rect.top && my <= rect.bottom)
        {
            TransferCurrentToSupply(hDlg,mx,my);
            cLastScrollboxUsed = LAST_USED_CURRENT;
        }
        break;

    //MEH Mousewheel makes current bitmaps scroll
    case WM_MOUSEWHEEL:
        if(nEditType != EDIT_OBJECTS)
            break;
        if((short)(HIWORD(wParam)) > 0)  //Rotated wheel up
        {
            DoScroll(SB_LINEUP, 0, hDlg,
                     GetDlgItem(hDlg,(cLastScrollboxUsed == LAST_USED_CURRENT)?(IDC_VSCROLLCURRENT):(IDC_VSCROLLSUPPLY)));
        }
        else    //Rotated wheel down
        {
            DoScroll(SB_LINEDOWN, 0, hDlg,
                     GetDlgItem(hDlg,(cLastScrollboxUsed == LAST_USED_CURRENT)?(IDC_VSCROLLCURRENT):(IDC_VSCROLLSUPPLY)));
        }
        break;

    case WM_VSCROLL:
        DoScroll(LOWORD(wParam),HIWORD(wParam),hDlg,(HWND)lParam);
        break;

    case WM_PAINT:
        ShowSupply(hDlg);
        ShowCurrent(hDlg);
        return(FALSE);

    case WM_COMMAND:
        switch (wParam)
            {
            case IDOK:
                if (nEditType == EDIT_OBJECTS)
                    {
                    GetObjectEdit(hDlg);
                    CheckObject(nCurrentType);
                    SetUndoPoint();
                    }
                CloseWinG();
                EndDialog(hDlg, TRUE);
                break;

            case IDCANCEL:
                CloseWinG();
                EndDialog(hDlg, FALSE);
                break;

            case IDC_LOAD:
                LoadNewSupply(hDlg);
                break;

            case IDC_CREATE:
                SetNewObjectSequence(hDlg,&pObj->Create,NO_CREATE);
                break;

            case IDC_WALK:
                SetNewObjectSequence(hDlg,&pObj->Walk,NO_WALK);
                break;

            case IDC_INTERACT:
                SetNewObjectSequence(hDlg,&pObj->Interact,NO_INTERACT);
                break;

            case IDC_ATTACK:
                SetNewObjectSequence(hDlg,&pObj->Attack,NO_ATTACK);
                break;

            case IDC_DESTROY:
                SetNewObjectSequence(hDlg,&pObj->Destroy,NO_DESTROY);
                break;

            case IDC_ACTIVE:
                pObj->Active ^= 1;
                SetObjectControls(hDlg);
                break;

            case IDC_NORMAL:
                nWallFlags = 0;
                SetWallControls(hDlg);
                break;

            case IDC_DOORSPLIT:
                nWallFlags &= ~DOOR_TYPE_SLIDE;
                nWallFlags &= ~DOOR_TYPE_SECRET;
                nWallFlags |= DOOR_TYPE_SPLIT;
                SetWallControls(hDlg);
                break;

            case IDC_DOORSLIDING:
                nWallFlags &= ~DOOR_TYPE_SPLIT;
                nWallFlags &= ~DOOR_TYPE_SECRET;
                nWallFlags |= DOOR_TYPE_SLIDE;
                SetWallControls(hDlg);
                break;

            case IDC_DOORSECRET:
                nWallFlags &= ~DOOR_TYPE_SPLIT;
                nWallFlags &= ~DOOR_TYPE_SLIDE;
                nWallFlags |= DOOR_TYPE_SECRET;
                SetWallControls(hDlg);
                break;

            case IDC_TRANSPARENT:
                nWallFlags ^= WALL_TYPE_TRANS;
                SetWallControls(hDlg);
                break;

            case IDC_WALLPASS:
                nWallFlags ^= WALL_TYPE_PASS;
                SetWallControls(hDlg);
                break;

            case IDC_MULTIVIEW:
                pCurrentOS->flags ^= OF_MULTIVIEW;
                SetObjectControls(hDlg);
                break;

            case IDC_ANIMATE:
                pCurrentOS->flags ^= OF_ANIMATE;
                SetObjectControls(hDlg);
                break;

            case IDC_ANIMONCE:
                pCurrentOS->flags ^= OF_ANIMONCE;
                SetObjectControls(hDlg);
                break;

            case IDC_MOVEABLE:
                pCurrentOS->flags ^= OF_MOVEABLE;
                SetObjectControls(hDlg);
                break;

            case IDC_OBJPASS:
                pCurrentOS->flags ^= OF_PASSABLE;
                SetObjectControls(hDlg);
                break;

            default:
                return(FALSE);
            }
        break;

    default:
        return(FALSE);
    }

return(TRUE);
}

//**************************************************************************
//
//**************************************************************************
short ReadTempInfo(HWND hDlg)
{
    FILE    *fp;
    char    *s;
    char    szBuf[256];

fp = fopen("ACKTEMP.ATF","rt");
if (fp == NULL)
    {
    MessageBox(hDlg,"Unable to read .ATF file","Error",MB_OK);
    return(-1);
    }

while (1)
    {
    if (feof(fp))
        break;

    *szBuf = '\0';
    fgets(szBuf,255,fp);
    if (*szBuf)
        {
        s = strchr(szBuf,13);
        if (s != NULL)
            *s = '\0';
        s = strchr(szBuf,10);
        if (s != NULL)
            *s = '\0';
        SendDlgItemMessage(hDlg,IDC_LISTBOX1,LB_ADDSTRING,0,(LPARAM)szBuf);
        }
    }

fclose(fp);
unlink("ACKTEMP.ATF");

return(0);
}

//**************************************************************************
//
//**************************************************************************
BOOL CALLBACK _export FAR PASCAL InfoDlgProc(HWND hDlg, UINT iMessage,
        WPARAM wParam, LPARAM lParam)
{

switch (iMessage)
    {
    case WM_INITDIALOG:
        ReadTempInfo(hDlg);
        return(TRUE);

    case WM_COMMAND:
        switch (wParam)
            {
            case IDOK:
                EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

            default:
                return(FALSE);
            }
        break;

    default:
        return(FALSE);
    }

return(TRUE);
}


