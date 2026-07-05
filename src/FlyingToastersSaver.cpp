/*
 * Haiku BScreenSaver port of flying-toasters.c, with a config panel.
 *
 * Toaster/toast counts and speed are runtime-adjustable via sliders in
 * StartConfig(), persisted through SaveState()/the constructor's archive
 * argument. To keep this allocation-free, arrays are sized to a fixed
 * MAX_* capacity and an "active count" member controls how many of each
 * are actually spawned/drawn.
 *
 * Build as a shared object (add-on), not an executable. See the
 * accompanying Makefile.
 */

#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL.h>

#include <ScreenSaver.h>
#include <View.h>
#include <Bitmap.h>
#include <Slider.h>
#include <StringView.h>
#include <Message.h>

#include "../img/toast.xpm"
#include "../img/toaster.xpm"
#include "xpm.h"
#include "flying-toasters.h"

#define TOASTER_SPRITE_COUNT 6
#define SPRITE_SIZE 64

#define MAX_TOASTER_COUNT 20
#define MAX_TOAST_COUNT   12
#define GRID_WIDTH  8
#define GRID_HEIGHT 4   /* GRID_WIDTH * GRID_HEIGHT must == MAX_TOASTER_COUNT + MAX_TOAST_COUNT */

#define DEFAULT_TOASTER_COUNT 10
#define DEFAULT_TOAST_COUNT   6
#define DEFAULT_SPEED_LEVEL   4
#define MAX_SPEED_LEVEL       8

#define FPS 60

/* ---- helper functions, generalized to take counts instead of fixed macros ---- */

int hasSpriteCollision(int x1, int y1, int x2, int y2, int gap) {
    return (x1 < x2 + SPRITE_SIZE + gap) && (x2 < x1 + SPRITE_SIZE + gap) &&
           (y1 < y2 + SPRITE_SIZE + gap) && (y2 < y1 + SPRITE_SIZE + gap);
}

int isScrolledToScreen(int x, int y, int screenWidth) {
    return (y + SPRITE_SIZE > 0) && (x + SPRITE_SIZE > 0) && (x < screenWidth);
}

int isScrolledOutOfScreen(int x, int y, int screenHeight) {
    return (x <= -SPRITE_SIZE) || (y >= screenHeight);
}

void loadSprites(SDL_Renderer *renderer,
                 SDL_Texture **toasterTextures,
                 SDL_Texture **toastTexture) {
    for (int i = 0; i < TOASTER_SPRITE_COUNT; i++) {
        SDL_Surface *surf = xpm_to_surface((const char *const *)toasterXpm[i]);
        if (!surf) {
            for (int j = 0; j < i; j++) SDL_DestroyTexture(toasterTextures[j]);
            *toastTexture = NULL;
            return;
        }
        toasterTextures[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }
    SDL_Surface *toastSurf = xpm_to_surface((const char *const *)toastXpm);
    if (!toastSurf) {
        for (int i = 0; i < TOASTER_SPRITE_COUNT; i++) SDL_DestroyTexture(toasterTextures[i]);
        *toastTexture = NULL;
        return;
    }
    *toastTexture = SDL_CreateTextureFromSurface(renderer, toastSurf);
    SDL_FreeSurface(toastSurf);
}

void freeSprites(SDL_Texture **toasterTextures, SDL_Texture *toastTexture) {
    for (int i = 0; i < TOASTER_SPRITE_COUNT; i++) {
        SDL_DestroyTexture(toasterTextures[i]);
    }
    SDL_DestroyTexture(toastTexture);
}

void setToasterSpawnCoordinates(struct Toaster *toaster, int screenWidth, int screenHeight) {
    int slotWidth = screenWidth / GRID_WIDTH;
    int slotHeight = screenHeight / GRID_HEIGHT;
    toaster->x = screenHeight + (toaster->slot % GRID_WIDTH) * slotWidth + (slotWidth - SPRITE_SIZE) / 2;
    toaster->y = -screenHeight + (toaster->slot / GRID_WIDTH) * slotHeight + (slotHeight - SPRITE_SIZE) / 2;
}

void setToastSpawnCoordinates(struct Toast *toast, int screenWidth, int screenHeight) {
    int slotWidth = screenWidth / GRID_WIDTH;
    int slotHeight = screenHeight / GRID_HEIGHT;
    toast->x = screenHeight + (toast->slot % GRID_WIDTH) * slotWidth + (slotWidth - SPRITE_SIZE) / 2;
    toast->y = -screenHeight + (toast->slot / GRID_WIDTH) * slotHeight + (slotHeight - SPRITE_SIZE) / 2;
}

void spawnToasters(struct Toaster *toasters, int count, int screenWidth, int screenHeight,
                    const int *grid, int maxSpeed) {
    for (int i = 0; i < count; i++) {
        toasters[i].slot = grid[i];
        toasters[i].moveDistance = 1 + rand() % maxSpeed;
        toasters[i].currentFrame = rand() % TOASTER_SPRITE_COUNT;
        setToasterSpawnCoordinates(&toasters[i], screenWidth, screenHeight);
    }
}

void spawnToasts(struct Toast *toasts, int count, int screenWidth, int screenHeight,
                  const int *grid, int maxSpeed) {
    for (int i = 0; i < count; i++) {
        /* offset by MAX_TOASTER_COUNT (not the active count) so toast
         * slots never collide with toaster slots regardless of the
         * current slider values */
        toasts[i].slot = grid[MAX_TOASTER_COUNT + i];
        toasts[i].moveDistance = 1 + rand() % maxSpeed;
        setToastSpawnCoordinates(&toasts[i], screenWidth, screenHeight);
    }
}

void drawSprite(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y) {
    if (!texture) return;
    SDL_Rect dst = { x, y, SPRITE_SIZE, SPRITE_SIZE };
    SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void initGridInto(int *grid) {
    const int total = MAX_TOASTER_COUNT + MAX_TOAST_COUNT;
    for (int i = 0; i < total; i++) grid[i] = i;
    for (int i = 0; i < total - 1; i++) {
        int j = i + rand() % (total - i);
        int t = grid[j];
        grid[j] = grid[i];
        grid[i] = t;
    }
}

static int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ---- Haiku BScreenSaver ---- */

class FlyingToastersSaver;

enum {
    MSG_TOASTER_COUNT = 'ftct',
    MSG_TOAST_COUNT   = 'ftsc',
    MSG_SPEED         = 'ftsp',
};

class ConfigView : public BView {
public:
    ConfigView(BRect frame, FlyingToastersSaver *saver);
    void AttachedToWindow() override;
    void MessageReceived(BMessage *msg) override;

private:
    FlyingToastersSaver *fSaver;
    BSlider *fToasterSlider;
    BSlider *fToastSlider;
    BSlider *fSpeedSlider;
};

class FlyingToastersSaver : public BScreenSaver {
public:
    FlyingToastersSaver(BMessage *archive, image_id id)
        : BScreenSaver(archive, id),
          fSurface(NULL),
          fRenderer(NULL),
          fToastTexture(NULL),
          fBitmap(NULL),
          fFrameCounter(0),
          fWidth(0),
          fHeight(0),
          fToasterCount(DEFAULT_TOASTER_COUNT),
          fToastCount(DEFAULT_TOAST_COUNT),
          fSpeedLevel(DEFAULT_SPEED_LEVEL) {
        memset(fToasterTextures, 0, sizeof(fToasterTextures));

        int32 v;
        if (archive && archive->FindInt32("toaster_count", &v) == B_OK)
            fToasterCount = v;
        if (archive && archive->FindInt32("toast_count", &v) == B_OK)
            fToastCount = v;
        if (archive && archive->FindInt32("speed_level", &v) == B_OK)
            fSpeedLevel = v;

        fToasterCount = clampInt(fToasterCount, 1, MAX_TOASTER_COUNT);
        fToastCount   = clampInt(fToastCount, 1, MAX_TOAST_COUNT);
        fSpeedLevel   = clampInt(fSpeedLevel, 1, MAX_SPEED_LEVEL);
    }

    status_t StartSaver(BView *view, bool preview) override {
        srand((unsigned)time(NULL));

        BRect bounds = view->Bounds();
        fWidth  = (int)bounds.Width() + 1;
        fHeight = (int)bounds.Height() + 1;
        if (fWidth < 1) fWidth = 1;
        if (fHeight < 1) fHeight = 1;

        SetTickSize(1000000 / FPS);

        fSurface = SDL_CreateRGBSurfaceWithFormat(
            0, fWidth, fHeight, 32, SDL_PIXELFORMAT_BGRA32);
        if (!fSurface)
            return B_ERROR;

        fRenderer = SDL_CreateSoftwareRenderer(fSurface);
        if (!fRenderer) {
            SDL_FreeSurface(fSurface);
            fSurface = NULL;
            return B_ERROR;
        }

        loadSprites(fRenderer, fToasterTextures, &fToastTexture);
        if (!fToastTexture) {
            SDL_DestroyRenderer(fRenderer);
            SDL_FreeSurface(fSurface);
            fRenderer = NULL;
            fSurface = NULL;
            return B_ERROR;
        }

        initGridInto(fGrid);

        int maxToasterSpeed = fSpeedLevel;
        int maxToastSpeed   = fSpeedLevel > 1 ? fSpeedLevel - 1 : 1;

        spawnToasters(fToasters, fToasterCount, fWidth, fHeight, fGrid, maxToasterSpeed);
        spawnToasts(fToasts, fToastCount, fWidth, fHeight, fGrid, maxToastSpeed);

        fBitmap = new BBitmap(bounds, B_RGBA32);
        fFrameCounter = 0;

        return B_OK;
    }

    void StopSaver() override {
        if (fToastTexture)
            freeSprites(fToasterTextures, fToastTexture);
        if (fRenderer) SDL_DestroyRenderer(fRenderer);
        if (fSurface) SDL_FreeSurface(fSurface);
        delete fBitmap;
        fBitmap = NULL;
        fRenderer = NULL;
        fSurface = NULL;
        fToastTexture = NULL;
    }

    void Draw(BView *view, int32 frame) override {
        if (!fRenderer || !fBitmap) return;

        SDL_SetRenderDrawColor(fRenderer, 0, 0, 0, 255);
        SDL_RenderClear(fRenderer);

        fFrameCounter = (fFrameCounter + 1) % 256;

        for (int i = 0; i < fToastCount; i++) {
            if (isScrolledToScreen(fToasts[i].x, fToasts[i].y, fWidth)) {
                drawSprite(fRenderer, fToastTexture, fToasts[i].x, fToasts[i].y);
            }
            int newX = fToasts[i].x - fToasts[i].moveDistance;
            int newY = fToasts[i].y + fToasts[i].moveDistance;
            if (isScrolledOutOfScreen(newX, newY, fHeight)) {
                setToastSpawnCoordinates(&fToasts[i], fWidth, fHeight);
            } else {
                fToasts[i].x = newX;
                fToasts[i].y = newY;
            }
        }

        for (int i = 0; i < fToasterCount; i++) {
            if (isScrolledToScreen(fToasters[i].x, fToasters[i].y, fWidth)) {
                drawSprite(fRenderer, fToasterTextures[fToasters[i].currentFrame],
                           fToasters[i].x, fToasters[i].y);
            }
            int newX = fToasters[i].x - fToasters[i].moveDistance;
            int newY = fToasters[i].y + fToasters[i].moveDistance;
            if (isScrolledOutOfScreen(newX, newY, fHeight)) {
                setToasterSpawnCoordinates(&fToasters[i], fWidth, fHeight);
            } else {
                for (int j = 0; j < fToasterCount; j++) {
                    if (i != j && hasSpriteCollision(fToasters[j].x, fToasters[j].y, newX, newY, 0)) {
                        if (fToasters[i].x <= fToasters[j].x + SPRITE_SIZE) {
                            newY = fToasters[i].y + fToasters[j].moveDistance;
                        } else {
                            newX = fToasters[i].x - fToasters[j].moveDistance;
                        }
                        break;
                    }
                }
                fToasters[i].x = newX;
                fToasters[i].y = newY;
            }
            if (fFrameCounter % (10 - fToasters[i].moveDistance) == 0) {
                fToasters[i].currentFrame = (fToasters[i].currentFrame + 1) % TOASTER_SPRITE_COUNT;
            }
        }

        SDL_RenderPresent(fRenderer);

        /* Both SDL_PIXELFORMAT_BGRA32 and SDL_PIXELFORMAT_RGBA32 produced
         * the same red/blue-swapped result on this system, so rather than
         * keep guessing at which SDL enum "should" match B_RGBA32, swap
         * the R and B bytes directly before handing pixels to Haiku. */
        {
            uint8_t *px = (uint8_t *)fSurface->pixels;
            int totalPixels = fWidth * fHeight;
            for (int i = 0; i < totalPixels; i++) {
                uint8_t tmp = px[i * 4];
                px[i * 4] = px[i * 4 + 2];
                px[i * 4 + 2] = tmp;
            }
        }

        fBitmap->SetBits(fSurface->pixels, fSurface->pitch * fHeight, 0, B_RGBA32);
        view->DrawBitmap(fBitmap, BPoint(0, 0));
    }

    void StartConfig(BView *view) override {
        view->AddChild(new ConfigView(view->Bounds(), this));
    }

    status_t SaveState(BMessage *into) const override {
    into->RemoveName("toaster_count");
    into->AddInt32("toaster_count", fToasterCount);
    into->RemoveName("toast_count");
    into->AddInt32("toast_count", fToastCount);
    into->RemoveName("speed_level");
    into->AddInt32("speed_level", fSpeedLevel);
    return B_OK;

    }

    /* Accessors used by ConfigView. These are only ever touched by the
     * config-panel instance of this class (a separate object from the
     * one that actually runs on the real screen), so there's no
     * cross-thread contention with Draw(). */
    int ToasterCount() const { return fToasterCount; }
    int ToastCount() const { return fToastCount; }
    int SpeedLevel() const { return fSpeedLevel; }

    void SetToasterCount(int v) { fToasterCount = clampInt(v, 1, MAX_TOASTER_COUNT); }
    void SetToastCount(int v) { fToastCount = clampInt(v, 1, MAX_TOAST_COUNT); }
    void SetSpeedLevel(int v) { fSpeedLevel = clampInt(v, 1, MAX_SPEED_LEVEL); }

private:
    SDL_Surface  *fSurface;
    SDL_Renderer *fRenderer;
    SDL_Texture  *fToasterTextures[TOASTER_SPRITE_COUNT];
    SDL_Texture  *fToastTexture;
    BBitmap      *fBitmap;

    struct Toaster fToasters[MAX_TOASTER_COUNT];
    struct Toast   fToasts[MAX_TOAST_COUNT];
    int fGrid[MAX_TOASTER_COUNT + MAX_TOAST_COUNT];

    int fFrameCounter;
    int fWidth, fHeight;

    int fToasterCount;
    int fToastCount;
    int fSpeedLevel;
};

ConfigView::ConfigView(BRect frame, FlyingToastersSaver *saver)
    : BView(frame, "flying-toasters-config", B_FOLLOW_ALL, B_WILL_DRAW),
      fSaver(saver) {
    SetViewColor(216, 216, 216);

    BRect r(20, 20, frame.Width() - 20, 50);
    fToasterSlider = new BSlider(r, "toasters", "Toasters",
        new BMessage(MSG_TOASTER_COUNT), 1, MAX_TOASTER_COUNT, B_BLOCK_THUMB);
    fToasterSlider->SetValue(saver->ToasterCount());
    fToasterSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fToasterSlider->SetHashMarkCount(MAX_TOASTER_COUNT);
    fToasterSlider->SetLimitLabels("1", "20");
    AddChild(fToasterSlider);

    r.OffsetBy(0, 60);
    fToastSlider = new BSlider(r, "toasts", "Toasts",
        new BMessage(MSG_TOAST_COUNT), 1, MAX_TOAST_COUNT, B_BLOCK_THUMB);
    fToastSlider->SetValue(saver->ToastCount());
    fToastSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fToastSlider->SetHashMarkCount(MAX_TOAST_COUNT);
    fToastSlider->SetLimitLabels("1", "12");
    AddChild(fToastSlider);

    r.OffsetBy(0, 60);
    fSpeedSlider = new BSlider(r, "speed", "Speed",
        new BMessage(MSG_SPEED), 1, MAX_SPEED_LEVEL, B_BLOCK_THUMB);
    fSpeedSlider->SetValue(saver->SpeedLevel());
    fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fSpeedSlider->SetHashMarkCount(MAX_SPEED_LEVEL);
    fSpeedSlider->SetLimitLabels("Slow", "Fast");
    AddChild(fSpeedSlider);

    r.OffsetBy(0, 50);
    BStringView *note1 = new BStringView(r, "note1",
        "Close and reopen the preview");
    note1->SetFontSize(10);
    AddChild(note1);

    r.OffsetBy(0, 16);
    BStringView *note2 = new BStringView(r, "note2",
        "to apply changes.");
    note2->SetFontSize(10);
    AddChild(note2);


}

void ConfigView::AttachedToWindow() {
    fToasterSlider->SetTarget(this);
    fToastSlider->SetTarget(this);
    fSpeedSlider->SetTarget(this);
}

void ConfigView::MessageReceived(BMessage *msg) {
    switch (msg->what) {
        case MSG_TOASTER_COUNT:
            fSaver->SetToasterCount(fToasterSlider->Value());
            break;
        case MSG_TOAST_COUNT:
            fSaver->SetToastCount(fToastSlider->Value());
            break;
        case MSG_SPEED:
            fSaver->SetSpeedLevel(fSpeedSlider->Value());
            break;
        default:
            BView::MessageReceived(msg);
    }
}

extern "C" _EXPORT BScreenSaver *
instantiate_screen_saver(BMessage *archive, image_id id)
{
    return new FlyingToastersSaver(archive, id);
}
