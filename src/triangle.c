#include "triangle.h"
#include "display.h"
#include "texture.h"

// Draw a filled triangle with a flat bottom
void fill_flat_bottom_triangle(triangle_t triangle, color_t color)
{
  int x0 = triangle.points[0].x;
  int y0 = triangle.points[0].y;
  int x1 = triangle.points[1].x;
  int y1 = triangle.points[1].y;
  int x2 = triangle.points[2].x;
  int y2 = triangle.points[2].y;

  float inv_slope1 = y1 != y0 ? ((float)(x1 - x0)) / abs((int)y1 - (int)y0) : 0; // Inverse slope (run over rise), because in this case y is the independent value
  float inv_slope2 = y2 != y0 ? ((float)(x2 - x0)) / abs((int)y2 - (int)y0) : 0;

  // We cannot linearly interpolate w, so we interpolate the reciprocal of w instead which is linear.
  // Get the reciprocal all of the ws of the points
  // w is initially the z component
  float inv_w_a = 1 / triangle.points[0].w;
  float inv_w_b = 1 / triangle.points[1].w;
  float inv_w_c = 1 / triangle.points[2].w;

  if (y0 != y1)
  {
    for (int yi = y0; yi <= y1; yi++)
    {
      int x_start = x1 + (yi - y1) * inv_slope1;
      int x_end = x0 + (yi - y0) * inv_slope2;
      if (x_start > x_end)
      {
        float temp = x_start;
        x_start = x_end;
        x_end = temp;
      }
      for (int xi = x_start; xi < x_end; xi++)
      {
        // We cannot use draw_line here as we need to sample the color of the texture
        draw_triangle_pixel(xi, yi, triangle.points[0], triangle.points[1], triangle.points[2], inv_w_a, inv_w_b, inv_w_c, color);
      }
    }
  }
}

//------------------------------------------------------//
// Draw a filled triangle with a flat top               //
//------------------------------------------------------//
//  (x0,y0)------(x1,y1)                                //
//     \            /                                   //
//      \          /                                    //
//       \        /                                     //
//        \      /                                      //
//         \    /                                       //
//          \  /                                        //
//           \/                                         //
//        (x2,y2)                                       //
//------------------------------------------------------//
void fill_flat_top_triangle(triangle_t triangle, color_t color)
{
  int x0 = triangle.points[0].x;
  int y0 = triangle.points[0].y;
  int x1 = triangle.points[1].x;
  int y1 = triangle.points[1].y;
  int x2 = triangle.points[2].x;
  int y2 = triangle.points[2].y;

  float inv_slope1 = y2 != y1 ? ((float)(x2 - x1)) / abs(y2 - y1) : 0; // Inverse slope (run over rise), because in this case y is the independent value
  float inv_slope2 = y2 != y0 ? ((float)(x2 - x0)) / abs(y2 - y0) : 0;

  // We cannot linearly interpolate w, so we interpolate the reciprocal of w instead which is linear.
  // Get the reciprocal all of the ws of the points
  // w is initially the z component
  float inv_w_a = 1 / triangle.points[0].w;
  float inv_w_b = 1 / triangle.points[1].w;
  float inv_w_c = 1 / triangle.points[2].w;

  for (int yi = y1; yi <= y2; yi++)
  {
    int x_start = x1 + (yi - y1) * inv_slope1;
    int x_end = x0 + (yi - y0) * inv_slope2;
    if (x_start > x_end)
    {
      float temp = x_start;
      x_start = x_end;
      x_end = temp;
    }
    for (int xi = x_start; xi < x_end; xi++)
    {
      // We cannot use draw_line here as we need to sample the color of the texture
      draw_triangle_pixel(xi, yi, triangle.points[0], triangle.points[1], triangle.points[2], inv_w_a, inv_w_b, inv_w_c, color);
    }
    x_start -= inv_slope1;
    x_end -= inv_slope2;
  }
}

void draw_filled_triangle(triangle_t triangle, color_t color)
{
  sort_three_vertices_uv_by_y(&triangle);
  int y0 = triangle.points[0].y;
  int y1 = triangle.points[1].y;
  int y2 = triangle.points[2].y;

  // If one of the y values are equal, then that means that the triangle does not need to be divided into two!
  if (y0 != y1)
  {
    fill_flat_bottom_triangle(triangle, color);
  }
  if (y1 != y2)
  {
    fill_flat_top_triangle(triangle, color);
  }
  return;
}

void draw_triangle_pixel(int xi, int yi,
                         vec4_t point_a, vec4_t point_b, vec4_t point_c,
                         float inv_w_a, float inv_w_b, float inv_w_c,
                         color_t color)
{
  vec2_t p = {.x = xi, .y = yi};
  vec3_t weights = barycentric_weights(vec4_xy(point_a), vec4_xy(point_b), vec4_xy(point_c), p);

  float alpha = weights.x;
  float beta = weights.y;
  float gamma = weights.z;

  // Interpolate using barycentric coordinates, multiplying by 1 / w of the point for correct perspective texture mapping (instead of affine texture mapping)
  float inverse_w = inv_w_a * alpha + inv_w_b * beta + inv_w_c * gamma;

  // Using the inverse w as is is incorrect, as nearer objects get lesser inverse w values than farther ones.
  // As such, we need to subtract the inverse w from 1.0.
  float transformed_inverse_w = 1.0 - inverse_w;
  if (transformed_inverse_w < z_buffer[(window_width * yi) + xi]) // Draw only if the current depth is less than what is in the z-buffer
  {
    draw_pixel(xi, yi, color);
    z_buffer[(window_width * yi) + xi] = transformed_inverse_w;
  }
}

void draw_texel(int xi, int yi,
                vec4_t point_a, vec4_t point_b, vec4_t point_c,
                float inv_w_a, float inv_w_b, float inv_w_c,
                tex2_t uv_a, tex2_t uv_b, tex2_t uv_c, color_t *texture)
{
  vec2_t p = {.x = xi, .y = yi};
  vec3_t weights = barycentric_weights(vec4_xy(point_a), vec4_xy(point_b), vec4_xy(point_c), p);

  float alpha = weights.x;
  float beta = weights.y;
  float gamma = weights.z;

  // Interpolate using barycentric coordinates, multiplying by 1 / w of the point for correct perspective texture mapping (instead of affine texture mapping)
  float u = uv_a.u * inv_w_a * alpha + uv_b.u * inv_w_b * beta + uv_c.u * inv_w_c * gamma;
  float v = uv_a.v * inv_w_a * alpha + uv_b.v * inv_w_b * beta + uv_c.v * inv_w_c * gamma;
  float inverse_w = inv_w_a * alpha + inv_w_b * beta + inv_w_c * gamma;

  // Divide by the interpolated inverse of w to remove the initial division by w.
  u /= inverse_w;
  v /= inverse_w;

  // Map the UV coordinate to actual texture dimensions, wrapping and clamping to avoid overflow
  int texture_x = clamp(0, texture_width, abs((int)(texture_width * u)) % texture_width);
  int texture_y = clamp(0, texture_height, abs((int)(texture_height * v)) % texture_height);

  // Using the inverse w as is is incorrect, as nearer objects get lesser inverse w values than farther ones.
  // As such, we need to subtract the inverse w from 1.0.
  float transformed_inverse_w = 1.0 - inverse_w;
  if (transformed_inverse_w < z_buffer[(window_width * yi) + xi]) // Draw only if the current depth is less than what is in the z-buffer
  {
    draw_pixel(xi, yi, texture[texture_width * texture_y + texture_x]);
    z_buffer[(window_width * yi) + xi] = transformed_inverse_w;
  }
}

void sort_three_vertices_uv_by_y(triangle_t *triangle) // Insertion Sort, sorts in place
{
  for (int slot = 1; slot < 3; slot++)
  {
    int i = slot - 1;
    vec4_t vertex = triangle->points[i + 1];
    tex2_t uv = triangle->texcoords[i + 1];

    while (i >= 0 && triangle->points[i].y > vertex.y) // Sort by y-value
    {
      triangle->points[i + 1] = triangle->points[i];
      triangle->texcoords[i + 1] = triangle->texcoords[i];
      i--;
    }
    triangle->points[i + 1] = vertex;
    triangle->texcoords[i + 1] = uv;
  }
}

void draw_textured_triangle(
    triangle_t triangle,
    color_t *texture)
{
  // Sort the vertices
  sort_three_vertices_uv_by_y(&triangle);

  // OBJ file UV coordinates start from the top left and v grows downwards,
  // so we need to change the v component to be make it start from the bottom left instead.
  for (int i = 0; i < 3; i++)
  {
    triangle.texcoords[i].v = 1.0 - triangle.texcoords[i].v;
  }

  // Unpack values
  vec4_t point_a = triangle.points[0];
  tex2_t uv_a = triangle.texcoords[0];
  vec4_t point_b = triangle.points[1];
  tex2_t uv_b = triangle.texcoords[1];
  vec4_t point_c = triangle.points[2];
  tex2_t uv_c = triangle.texcoords[2];

  int x0 = point_a.x;
  int y0 = point_a.y;

  int x1 = point_b.x;
  int y1 = point_b.y;

  int x2 = point_c.x;
  int y2 = point_c.y;

  // Draw flat top
  float inv_slope1 = y2 != y1 ? ((float)(x2 - x1)) / abs(y2 - y1) : 0; // Inverse slope (run over rise), because in this case y is the independent value
  float inv_slope2 = y2 != y0 ? ((float)(x2 - x0)) / abs(y2 - y0) : 0;

  // We cannot linearly interpolate w, so we interpolate the reciprocal of w instead which is linear.
  // Get the reciprocal all of the ws of the points
  // w is initially the z component
  float inv_w_a = 1 / point_a.w;
  float inv_w_b = 1 / point_b.w;
  float inv_w_c = 1 / point_c.w;

  if (y2 != y1)
  {
    for (int yi = y1; yi <= y2; yi++)
    {
      int x_start = x1 + (yi - y1) * inv_slope1;
      int x_end = x0 + (yi - y0) * inv_slope2;
      if (x_start > x_end)
      {
        float temp = x_start;
        x_start = x_end;
        x_end = temp;
      }
      for (int xi = x_start; xi < x_end; xi++)
      {
        // We cannot use draw_line here as we need to sample the color of the texture
        draw_texel(xi, yi, point_a, point_b, point_c,
                   inv_w_a, inv_w_b, inv_w_c,
                   uv_a, uv_b, uv_c, texture);
      }
      x_start -= inv_slope1;
      x_end -= inv_slope2;
    }
  }

  // Draw flat bottom
  inv_slope1 = y1 != y0 ? ((float)(x1 - x0)) / abs((int)y1 - (int)y0) : 0; // Inverse slope (run over rise), because in this case y is the independent value
  inv_slope2 = y2 != y0 ? ((float)(x2 - x0)) / abs((int)y2 - (int)y0) : 0;

  if (y0 != y1)
  {
    for (int yi = y0; yi <= y1; yi++)
    {
      int x_start = x1 + (yi - y1) * inv_slope1;
      int x_end = x0 + (yi - y0) * inv_slope2;
      if (x_start > x_end)
      {
        float temp = x_start;
        x_start = x_end;
        x_end = temp;
      }
      for (int xi = x_start; xi < x_end; xi++)
      {
        // We cannot use draw_line here as we need to sample the color of the texture
        draw_texel(xi, yi, point_a, point_b, point_c,
                   inv_w_a, inv_w_b, inv_w_c,
                   uv_a, uv_b, uv_c, texture);
      }
    }
  }

  return;
}

// Barycentric Coordinates
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p)
{
  vec2_t ac = vec2_sub(c, a);
  vec2_t ab = vec2_sub(b, a);
  vec2_t ap = vec2_sub(p, a);
  vec2_t pc = vec2_sub(c, p);
  vec2_t pb = vec2_sub(b, p);

  // Area of the parallelogram formed by the entire triangle
  // What we are actually computing here is the z component for what is supposed to be the cross product
  float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x); // || AC x AB ||

  float alpha = (pc.x * pb.y - pc.y * pb.x) / area_parallelogram_abc;
  float beta = (ac.x * ap.y - ac.y * ap.x) / area_parallelogram_abc;
  float gamma = 1 - alpha - beta;

  vec3_t barycentric_coordinates = {
      alpha, beta, gamma};
  return barycentric_coordinates;
}