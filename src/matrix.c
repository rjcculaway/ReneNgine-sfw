#include "matrix.h"

mat4_t mat4_identity(void)
{
  // | 1 0 0 0 |
  // | 0 1 0 0 |
  // | 0 0 1 0 |
  // | 0 0 0 1 |
  mat4_t m = {
      .m = {
          {1.0, 0.0, 0.0, 0.0},
          {0.0, 1.0, 0.0, 0.0},
          {0.0, 0.0, 1.0, 0.0},
          {0.0, 0.0, 0.0, 1.0},
      }};

  return m;
}

mat4_t mat4_make_scale(float sx, float sy, float sz)
{
  // | sx 0  0  0 |
  // | 0  sy 0  0 |
  // | 0  0  sz 0 |
  // | 0  0  0  1 |
  mat4_t scale_mat = mat4_identity();

  scale_mat.m[0][0] = sx;
  scale_mat.m[1][1] = sy;
  scale_mat.m[2][2] = sz;

  return scale_mat;
}

mat4_t inline mat4_make_scale_uniform(float s)
{
  // | s 0 0 0 |
  // | 0 s 0 0 |
  // | 0 0 s 0 |
  // | 0 0 0 1 |
  return mat4_make_scale(s, s, s);
}

vec4_t mat4_matmul_vec(mat4_t m, vec4_t v)
{
  vec4_t result;

  result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
  result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
  result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
  result.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;

  return result;
}

mat4_t mat4_matmul_mat4(mat4_t a, mat4_t b)
{
  mat4_t product;

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      product.m[i][j] = 0;
      for (int k = 0; k < 4; k++)
      {
        product.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }

  return product;
}

mat4_t mat4_make_translation(float tx, float ty, float tz)
{
  // | 0  0  0  tx |
  // | 0  0  0  ty |
  // | 0  0  0  tz |
  // | 0  0  0   1 |
  mat4_t m = mat4_identity();
  m.m[0][3] = tx;
  m.m[1][3] = ty;
  m.m[2][3] = tz;
  return m;
}

mat4_t mat4_make_rotation_z(float angle)
{
  // | cos(a)  -sin(a)   0        0 |
  // | sin(a)   cos(a)   0        0 |
  // | 0        0        1        0 |
  // | 0        0        0        1 |
  mat4_t m = mat4_identity();
  float cos_angle = cos(angle);
  float sin_angle = sin(angle);

  m.m[0][0] = cos_angle;
  m.m[0][1] = -sin_angle;
  m.m[1][0] = sin_angle;
  m.m[1][1] = cos_angle;

  return m;
}

mat4_t mat4_make_rotation_x(float angle)
{
  // | 1        0        0        0 |
  // | 0        cos(a)  -sin(a)   0 |
  // | 0        sin(a)   cos(a)   0 |
  // | 0        0        0        1 |
  mat4_t m = mat4_identity();
  float cos_angle = cos(angle);
  float sin_angle = sin(angle);

  m.m[1][1] = cos_angle;
  m.m[1][2] = -sin_angle;
  m.m[2][1] = sin_angle;
  m.m[2][2] = cos_angle;

  return m;
}

mat4_t mat4_make_rotation_y(float angle)
{
  // | cos(a)   0        sin(a)   0 |
  // | 0        1        0        0 |
  // | -sin(a)  0        cos(a)   0 |
  // | 0        0        0        1 |
  mat4_t m = mat4_identity();
  float cos_angle = cos(angle);
  float sin_angle = sin(angle);

  // Rotation is inverted to maintain a counter-clockwise rotation
  m.m[0][0] = cos_angle;
  m.m[0][2] = sin_angle;
  m.m[2][0] = -sin_angle;
  m.m[2][2] = cos_angle;

  return m;
}

// Projection
mat4_t mat4_make_perspective(float fov, float aspect, float z_near, float z_far)
{
  // | a / f  0      0      0          |
  // | 0      1 / f  0      0          |
  // | 0      0      l     -l * z_near |
  // | 0      0      1      0          |
  mat4_t m = mat4_identity();

  float f = tan(fov / 2); // Half angle, will be inverted to show that the higher the fov, the smaller the objects
  float lambda = z_far / (z_far - z_near);

  m.m[0][0] = aspect / f;
  m.m[1][1] = 1 / f;
  m.m[2][2] = lambda;
  m.m[2][3] = -lambda * z_near; // Normalize the z values
  m.m[3][2] = 1.0;              // Store the unchanged z in w
  m.m[3][3] = 0.0;

  return m;
}

vec4_t mat4_matmul_vec_project(mat4_t mat_proj, vec4_t v)
{
  vec4_t projected = mat4_matmul_vec(mat_proj, v);
  return projected;
}

mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up)
{
  // The idea behind a look at matrix is to move the camera to the origin.
  // In other words, move all vertices such that the camera IS the origin.
  // To do so, the "camera" must be moved to the origin and rotated such that it faces our +z (away from the monitor)

  // Multiplying the translation matrix by the INVERTED rotation matrix does that.
  // There is no need to perform actual inversion; since the rotation matrix is orthogonal (linearly independent),
  // all we need to do is transpose the rotation matrix. This transpose will already be baked into the created matrix.

  // To create the look at matrix, we need the forward (z), up (y), and right (x) vectors.
  vec3_t z = vec3_normalize(vec3_sub(target, eye));
  vec3_t x = vec3_normalize(vec3_cross(up, z));
  vec3_t y = vec3_cross(z, x);

  // | x.x  x.y  x.z  -dot(x, eye) |
  // | y.x  y.y  y.z  -dot(y, eye) |
  // | z.x  z.y  z.z  -dot(z, eye) |
  // |   0    0    0             1 |

  mat4_t look_at_matrix = {{{x.x, x.y, x.z, -vec3_dot(x, eye)},
                            {y.x, y.y, y.z, -vec3_dot(y, eye)},
                            {z.x, z.y, z.z, -vec3_dot(z, eye)},
                            {0, 0, 0, 1}}};

  return look_at_matrix;
}