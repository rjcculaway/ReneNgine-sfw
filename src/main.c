#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL.h>

#include "utils.h"
#include "array.h"
#include "vector.h"
#include "matrix.h"
#include "display.h"
#include "clipping.h"
#include "mesh.h"
#include "camera.h"
#include "light.h"

bool is_running = false;
uint64_t previous_frame_time = 0;
float delta_time = 0;

#define MAX_TRIANGLES_PER_MESH 10000
triangle_t triangles_to_render[MAX_TRIANGLES_PER_MESH];
int num_triangles_to_render = 0;

//--------------------------------------------
// Global transformation matrices
//--------------------------------------------
mat4_t world_matrix;
mat4_t view_matrix;
mat4_t projection_matrix;

bool setup(void)
{
  set_backface_culling_option(CULLING_BACKFACE);
  set_render_method(RENDER_TRIANGLE);

  // Setup the projection matrix
  float aspect_x = (float)get_window_width() / (float)get_window_height();
  float aspect_y = (float)get_window_height() / (float)get_window_width();
  float fovy = M_PI / 3.0; // the same as 180/3, or 60deg
  float fovx = fovx_from_fovy(fovy, aspect_x);
  float z_near = 1.0;
  float z_far = 20.0;

  projection_matrix = mat4_make_perspective(fovy, aspect_y, z_near, z_far);

  // Initialize the frustum planes
  initialize_frustum_planes(fovx, fovy, z_near, z_far);

  // Load mesh and texture data from .png file
  load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_create(1, 1, 1), vec3_create(0, -M_PI_2, 0), vec3_create(-1.5, 0.5, 5));
  load_mesh("./assets/efa.obj", "./assets/efa.png", vec3_create(1, 1, 1), vec3_create(0, -M_PI_2, 0), vec3_create(1.5, 0.5, 5));
  load_mesh("./assets/f117.obj", "./assets/f117.png", vec3_create(1, 1, 1), vec3_create(0, -M_PI_2, 0), vec3_create(0.0, 0.5, 3.0));
  load_mesh("./assets/runway.obj", "./assets/runway.png", vec3_create(1, 1, 1), vec3_create(0, M_PI, 0), vec3_create(0, 0, 10));

  mesh_t *meshes = get_meshes();
  size_t mesh_count = get_mesh_count();
  printf("Loaded %zd meshes.\n", mesh_count);
  for (size_t i = 0; i < mesh_count; i++)
  {
    printf("Mesh #%zd: vertices: %d, faces: %d, uvs: %d\n", i + 1, array_length(meshes[i].vertices), array_length(meshes[i].faces), array_length(meshes[i].texcoords));
  }

  // Initialize lights
  initialize_light(vec3_create(0, 0, 1));
  return true;
}

void process_input(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_QUIT: // Close button
      is_running = false;
      break;
    case SDL_KEYDOWN:
    {
      SDL_Keycode keycode = event.key.keysym.sym;
      if (keycode == SDLK_ESCAPE)
      {
        is_running = false;
        break;
      }
      if (keycode == SDLK_w)
      { // Move forward
        change_camera_forward_velocity_to_z(5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_s)
      { // Move backwards
        change_camera_forward_velocity_to_z(-5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_a)
      { // Move left
        change_camera_forward_velocity_to_x(-5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_d)
      { // Move right
        change_camera_forward_velocity_to_x(5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_q)
      {
        change_camera_forward_velocity_to_y(5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_e)
      {
        change_camera_forward_velocity_to_y(-5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_UP)
      { // Rotate up
        rotate_camera_pitch(-5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_DOWN)
      { // Rotate down
        rotate_camera_pitch(5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_LEFT)
      { // Rotate left
        rotate_camera_yaw(-5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_RIGHT)
      { // Rotate right
        rotate_camera_yaw(5.0 * delta_time);
        break;
      }
      if (keycode == SDLK_1)
      {
        set_render_method(RENDER_WIREFRAME_DOT);
        break;
      }
      if (keycode == SDLK_2)
      {
        set_render_method(RENDER_WIREFRAME);
        break;
      }
      if (keycode == SDLK_3)
      {
        set_render_method(RENDER_TRIANGLE);
        break;
      }
      if (keycode == SDLK_4)
      {
        set_render_method(RENDER_WIREFRAME_TRIANGLE);
        break;
      }
      if (keycode == SDLK_5)
      {
        set_render_method(RENDER_TEXTURED_TRIANGLE);
        break;
      }
      if (keycode == SDLK_6)
      {
        set_render_method(RENDER_TEXTURED_WIREFRAME_TRIANGLE);
        break;
      }
      if (keycode == SDLK_c)
      {
        set_backface_culling_option(CULLING_BACKFACE);
        break;
      }
      if (keycode == SDLK_x)
      {
        set_backface_culling_option(CULLING_NONE);
        break;
      }
    }
    break;
    case SDL_KEYUP:
      switch (event.key.keysym.sym)
      {
      case SDLK_w: // Move forward
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      case SDLK_s: // Move backwards
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      case SDLK_a: // Move left
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      case SDLK_d: // Move right
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      case SDLK_q:
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      case SDLK_e:
        set_camera_forward_velocity(vec3_create(0, 0, 0));
        break;
      }
      break;
    default:
      break;
    }
  }
}

void process_graphics_pipeline_stages(mesh_t *mesh)
{

  // +-------------+
  // | Model Space |  <----- Placing the mesh into the "world"
  // +-------------+

  // Create a scale, rotation, translation matrix
  mat4_t scale_mat = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.y);
  mat4_t rotation_x_mat = mat4_make_rotation_x(mesh->rotation.x);
  mat4_t rotation_y_mat = mat4_make_rotation_y(mesh->rotation.y);
  mat4_t rotation_z_mat = mat4_make_rotation_z(mesh->rotation.z);
  mat4_t translation_mat = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);

  // Create a world matrix to combine the three transformations
  world_matrix = mat4_identity();

  // Multiply the transformations to the world matrix
  world_matrix = mat4_matmul_mat4(scale_mat, world_matrix);
  world_matrix = mat4_matmul_mat4(rotation_z_mat, world_matrix);
  world_matrix = mat4_matmul_mat4(rotation_y_mat, world_matrix);
  world_matrix = mat4_matmul_mat4(rotation_x_mat, world_matrix);
  world_matrix = mat4_matmul_mat4(translation_mat, world_matrix);

  // +------------+
  // | View Space |  <------ Simulating a "camera"
  // +------------+

  // Update the camera position
  move_camera_by_forward_velocity();
  // Create a view matrix to transform the coordinate system to the camera's
  vec3_t target = {0, 0, 1};
  mat4_t camera_rotation_m = mat4_matmul_mat4(
      mat4_make_rotation_x(get_camera_pitch()),
      mat4_make_rotation_y(get_camera_yaw()));
  set_camera_direction(vec3_from_vec4(mat4_matmul_vec(camera_rotation_m, vec4_from_vec3(target))));

  // Offset the target
  target = get_camera_target();

  view_matrix = mat4_look_at(get_camera_position(), target, CAMERA_UP);

  // Multiply the world matrix by the view matrix to compose the transformations
  mat4_t view_world_matrix = mat4_matmul_mat4(view_matrix, world_matrix);

  int num_faces = array_length(mesh->faces);
  for (int i = 0; i < num_faces; i++)
  {
    face_t face = mesh->faces[i];

    vec3_t face_vertices[3];
    face_vertices[0] = mesh->vertices[face.a];
    face_vertices[1] = mesh->vertices[face.b];
    face_vertices[2] = mesh->vertices[face.c];

    vec4_t transformed_vertices[3];

    // Apply transformations to each vertex of this face
    for (int j = 0; j < 3; j++)
    {
      vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

      // Multiply the view-world matrix to the original vector
      transformed_vertex = mat4_matmul_vec(view_world_matrix, transformed_vertex);

      transformed_vertices[j] = transformed_vertex;
    }

    // +------------------+
    // | Backface Culling |  <------ Hiding backside faces that are not visible
    // +------------------+

    bool shouldCull = false;

    vec3_t face_normal = compute_triangle_normal(transformed_vertices);

    // The camera ray is the camera_position - vertex.
    // There is no need to use the camera position for the camera ray.
    // By the end of view matrix multiplication, the camera WILL be at the origin.
    vec3_t camera_ray = vec3_mul(vec3_from_vec4(transformed_vertices[0]), -1.0); // Vector from camera (now the origin) to point A

    float dot = vec3_dot(face_normal, camera_ray);

    switch (get_backface_culling_option())
    {
    case CULLING_NONE:
      break;
    case CULLING_BACKFACE:
      if (dot < 0.0) // If dot < 0, then it should not be rendered
      {
        shouldCull = true;
      }
      break;
    default:
      fprintf(stderr, "WARNING: Invalid culling option selected!");
      break;
    }
    if (shouldCull)
    {
      continue;
    }

    // +------------------------------+
    // | Frustum Culling and Clipping |  <------ Ignoring invisible triangles, clipping partially visible triangles, retaining visible triangles
    // +------------------------------+

    // Clip the vertices against the frustum planes before projection
    polygon_t polygon = polygon_from_triangle(
        vec3_from_vec4(transformed_vertices[0]),
        vec3_from_vec4(transformed_vertices[1]),
        vec3_from_vec4(transformed_vertices[2]),
        face.a_uv,
        face.b_uv,
        face.c_uv);

    clip_polygon(&polygon);

    // Break down the polygon back to triangle(s) if needed
    triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES];
    int num_triangles_after_clipping = 0;

    triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

    // Loop from all the triangles after clipping
    for (int t = 0; t < num_triangles_after_clipping; t++)
    {
      triangle_t clipped_triangle = triangles_after_clipping[t];
      vec4_t projected_points[3];
      for (int j = 0; j < 3; j++)
      {

        // +------------+
        // | Projection |  <------ Project to a "screen (2D)", simulating perspective
        // +------------+
        projected_points[j] = mat4_matmul_vec_project(projection_matrix, clipped_triangle.points[j]);

        // +-------------+
        // | Image Space |  <------ Apply perspective divide, mapping values from -1.0 to 1.0.
        // +-------------+

        // Perspective Divide
        if (projected_points[j].w != 0.0)
        {
          projected_points[j].x /= projected_points[j].w;
          projected_points[j].y /= projected_points[j].w;
          projected_points[j].z /= projected_points[j].w;
        }

        // Invert the y values since the y value in screen space grows downward from the top
        projected_points[j].y *= -1;

        // +--------------+
        // | Screen Space |  <------ Mapping values from [-1.0, 1.0] to [0, screen dimensions].
        // +--------------+

        // Scale into the view
        projected_points[j].x *= (get_window_width() / 2.0);
        projected_points[j].y *= (get_window_height() / 2.0);

        // Translate projected points to the middle of the screen
        projected_points[j].x += (get_window_width() / 2.0);
        projected_points[j].y += (get_window_height() / 2.0);
      }

      float light_intensity = light_lambertian(face_normal, get_sun_light().direction);
      color_t final_color = light_apply_intensity(face.color, light_intensity);

      // Prepare the final triangle to be rasterized
      triangle_t projected_triangle = {
          .points = {
              {.x = projected_points[0].x, .y = projected_points[0].y, .z = projected_points[0].z, .w = projected_points[0].w},
              {.x = projected_points[1].x, .y = projected_points[1].y, .z = projected_points[1].z, .w = projected_points[1].w},
              {.x = projected_points[2].x, .y = projected_points[2].y, .z = projected_points[2].z, .w = projected_points[2].w}},
          .texcoords = {{clipped_triangle.texcoords[0].u, clipped_triangle.texcoords[0].v}, {clipped_triangle.texcoords[1].u, clipped_triangle.texcoords[1].v}, {clipped_triangle.texcoords[2].u, clipped_triangle.texcoords[2].v}},
          .color = final_color,
          .texture = mesh->texture};
      // "Enqueue" the triangle for rendering
      if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH)
      {
        triangles_to_render[num_triangles_to_render++] = projected_triangle;
      }
    }
  }

  // Without the z-buffer, we would need to sort the triangles by z here (Painter's algorithm).
}

void update(void)
{
  // Determine if we still have time to wait before the next frame
  uint64_t time_to_wait = FRAME_TIME - (SDL_GetTicks64() - previous_frame_time);
  if (time_to_wait > 0 && time_to_wait <= FRAME_TIME)
  {
    SDL_Delay(time_to_wait); // Delay update until enough time has passed.
  }

  // Delta time is used for framerate independence
  delta_time = (float)(SDL_GetTicks64() - previous_frame_time) / MILLISECONDS_PER_SECOND;

  previous_frame_time = SDL_GetTicks64();

  memset(triangles_to_render, 0, sizeof(triangle_t) * MAX_TRIANGLES_PER_MESH);
  num_triangles_to_render = 0;

  // Loop through all of the meshes for processing
  size_t mesh_count = get_mesh_count();
  for (size_t mesh_idx = 0; mesh_idx < mesh_count; mesh_idx++)
  {
    mesh_t *mesh = get_mesh(mesh_idx);

    process_graphics_pipeline_stages(mesh);
  }
}

void render(void)
{
  clear_color_buffer(0xFF000000);
  clear_z_buffer();

  draw_grid();

  // Draw all projected points from the cube
  for (int i = 0; i < num_triangles_to_render; i++)
  {
    triangle_t triangle = triangles_to_render[i];
    int x0 = triangle.points[0].x;
    int y0 = triangle.points[0].y;

    int x1 = triangle.points[1].x;
    int y1 = triangle.points[1].y;

    int x2 = triangle.points[2].x;
    int y2 = triangle.points[2].y;

    switch (get_render_method())
    {
    case RENDER_WIREFRAME:
      draw_triangle(
          x0, y0,
          x1, y1,
          x2, y2,
          0xFFFFFFFF);
      break;
    case RENDER_WIREFRAME_DOT:
      draw_triangle(
          x0, y0,
          x1, y1,
          x2, y2,
          0xFFFFFFFF);
      const int point_size = 4;
      draw_rect(x0 - point_size / 2, y0 - point_size / 2, point_size, point_size, 0xFFFF0000);
      draw_rect(x1 - point_size / 2, y1 - point_size / 2, point_size, point_size, 0xFFFF0000);
      draw_rect(x2 - point_size / 2, y2 - point_size / 2, point_size, point_size, 0xFFFF0000);
      break;
    case RENDER_WIREFRAME_TRIANGLE:
      draw_filled_triangle(
          triangle,
          triangle.color);
      draw_triangle(
          x0, y0,
          x1, y1,
          x2, y2,
          0xFFFFFFFF);
      break;
    case RENDER_TRIANGLE:
      draw_filled_triangle(
          triangle,
          triangle.color);
      break;
    case RENDER_TEXTURED_TRIANGLE:
      draw_textured_triangle(
          triangle,
          triangle.texture);
      break;
    case RENDER_TEXTURED_WIREFRAME_TRIANGLE:
      draw_textured_triangle(
          triangle,
          triangle.texture);
      draw_triangle(
          x0, y0,
          x1, y1,
          x2, y2,
          0xFFFFFFFF);
      break;
    default:
      fprintf(stderr, "WARNING: Invalid render option selected!");
      break;
    }
  }

  render_color_buffer();
}

void free_resources(void)
{
  free_meshes();
}

int main(void)
{

  is_running = initialize_window();

  // Game loop:
  // 1. Process input.
  // 2. Update.
  // 3. Render.
  bool is_setup_success = setup();
  if (!is_setup_success)
  {
    {
      fprintf(stderr, "ERROR: Could not perform setup.\n");
      return 1;
    }
  }

  while (is_running)
  {
    process_input();
    update();
    render();
  }

  free_resources();
  destroy_window();

  return 0;
}