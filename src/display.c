#include "display.h"

int window_width = 800;
int window_height = 600;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
color_t *color_buffer = NULL; // Raw pixel data
float *z_buffer = NULL;
SDL_Texture *color_buffer_texture = NULL; // Texture to be displayed to the render target

size_t inline get_pixel(const size_t i, const size_t j)
{
  return (window_width * j) + i;
}

bool initialize_window(void)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    fprintf(stderr, "ERROR: Failed initializing SDL.\n");
    return false;
  }
  // Query the screen resolution
  SDL_DisplayMode display_mode;
  SDL_GetCurrentDisplayMode(0, &display_mode);

  window_width = display_mode.w;
  window_height = display_mode.h;
  // window_width = 800;
  // window_height = 600;

  // Create an SDL Window
  window = SDL_CreateWindow(
      NULL,                   // window title
      SDL_WINDOWPOS_CENTERED, // pos x
      SDL_WINDOWPOS_CENTERED, // pos y
      window_width,           // width
      window_height,          // height
      SDL_WINDOW_BORDERLESS);
  if (!window)
  {
    fprintf(stderr, "ERROR: Failed reating SDL window.\n");
    return false;
  }

  // Create an SDL renderer
  renderer = SDL_CreateRenderer(
      window, // pointer to window
      -1,     // default display device
      0);
  if (!renderer)
  {
    fprintf(stderr, "ERROR: Failed creating SDL renderer.\n");
    return false;
  }

  // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  return true;
}
void destroy_window(void)
{

  SDL_DestroyRenderer(renderer);
  renderer = NULL;

  SDL_DestroyWindow(window);
  window = NULL;

  SDL_Quit();
}

void render_color_buffer(void)
{
  SDL_UpdateTexture(
      color_buffer_texture,
      NULL,
      color_buffer,
      (int)(window_width * sizeof(color_t)));
  SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}
/**
 * Fills the buffer with a color.
 * @param color The color (in RGBA8888 format) to fill the color buffer with.
 */
void clear_color_buffer(color_t color)
{
  memset(color_buffer, color, sizeof(color_t) * window_width * window_height);
}

void clear_z_buffer(void)
{
  for (int j = 0; j < window_height; j++)
  {
    for (int i = 0; i < window_width; i++)
    {
      z_buffer[get_pixel(i, j)] = 1.0;
    }
  }
  // memset() isn't possible here since floats are multibyte types.
}

bool inline is_valid_pixel(int x, int y)
{
  return x < window_width && y < window_height && x >= 0 && y >= 0;
}
void draw_pixel(int x, int y, color_t color)
{
  if (is_valid_pixel(x, y))
  {
    color_buffer[get_pixel(x, y)] = color;
  }
}
void draw_rect(int x, int y, int width, int height, color_t color)
{
  for (int current_x = 0; current_x < width; current_x++)
  {
    for (int current_y = 0; current_y < height; current_y++)
    {
      draw_pixel(current_x + x, current_y + y, color);
    }
  }
}
void draw_grid(void)
{
  for (int y = 0; y < window_height; y++)
  {
    for (int x = 0; x < window_width; x++)
    {
      if (x % 50 == 0 || y % 50 == 0)
      {
        draw_pixel(x, y, 0xFF444444);
      }
    }
  }
}
// Digital Differential Analyzer algorithm for drawing lines
void draw_line(int x0, int y0, int x1, int y1, color_t color)
{
  int delta_x = x1 - x0;
  int delta_y = y1 - y0;

  // Get the longest side
  int longest_side_length = abs(delta_x) >= abs(delta_y) ? abs(delta_x) : abs(delta_y);

  // Find out how much we should increment in both x and y each step
  float x_inc = delta_x / (float)longest_side_length;
  float y_inc = delta_y / (float)longest_side_length;

  float x_i = x0;
  float y_i = y0;

  for (int i = 0; i <= longest_side_length; i++)
  {
    draw_pixel(round(x_i), round(y_i), color);
    x_i += x_inc;
    y_i += y_inc;
  }
}

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t color)
{
  draw_line(x0,
            y0,
            x1,
            y1,
            color);
  draw_line(x1,
            y1,
            x2,
            y2, color);
  draw_line(x2,
            y2,
            x0,
            y0, color);
}