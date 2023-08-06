//
// Created by Philip on 7/18/2023.
//

#pragma once

#include <vector>
#include "Texture.hpp"
#include "Shaders/FragmentShader.hpp"
#include "Shaders/VertexShader.hpp"
#include "FrameBuffer.hpp"
#include "Mesh.hpp"
#include "SkinnedMesh.hpp"
#include "../Physics/SDFCollision.hpp"

/**
 * Enable backface culling. Beware of winding order.
 */
const bool BACKFACE_CULLING = true;

/**
 * A Multithreading Software rasterizer
 */
class Renderer {
private:
    Camera camera;
    float near_plane{},far_plane{}; //todo combine

    /**
       * Get interpolated value
       * @param input Input values
       * @param uvw Barycentric coordinates
       * @return Interpolated value
       */
    template<int T> static glm::vec<T,float> applyBarycentric(const glm::vec<T,float> input[3],const glm::vec3& uvw){
        return input[0] * uvw.x + input[1] * uvw.y  + input[2] * uvw.z;
    }

    /**
     * Get interpolated value with perspective correction
     * @param input Input values to interpolate
     * @param uvw Barycentric Coordinates
     * @param clip_space Clip spaces vertex positions(homogeneous coordinates)
     * @return Interpolated value
     * @see https://computergraphics.stackexchange.com/questions/4079/perspective-correct-texture-mapping
     */
    template<int T> static glm::vec<T,float> applyBarycentricPerspective(const glm::vec<T,float> input[3],const glm::vec3& uvw,const glm::vec4 clip_space[3]){
        return ((input[0]/clip_space[0].w) * uvw.x + (input[1]/clip_space[1].w) * uvw.y  + (input[2]/clip_space[2].w) * uvw.z) / ((1.0f/clip_space[0].w) * uvw.x + (1.0f/clip_space[1].w) * uvw.y  + (1.0f/clip_space[2].w) * uvw.z);
    }

    /**
     * Check if a pixel is in a triangle
     * @param x,y Pixel coordinates
     * @param positions Screen space positions
     * @param depth Output pixel depth from triangle
     * @param uv Output barycentric coordinates
     * @return True if in triangle.
     * @see https://codeplea.com/triangular-interpolation
     * Adapted from my java version
     */
    static bool inTriangle(int x, int y, const glm::vec3 positions[3], float& depth, glm::vec3& uvw) {
        //Get barycentric coords
        float invDET = 1.0f/((positions[1].y-positions[2].y) * (positions[0].x-positions[2].x) +
                             (positions[2].x-positions[1].x) * (positions[0].y-positions[2].y));
        float w1 = ((positions[1].y-positions[2].y) * ((float)x-positions[2].x) + (positions[2].x-positions[1].x) * ((float)y-positions[2].y)) * invDET;
        float w2 = ((positions[2].y-positions[0].y) * ((float)x-positions[2].x) + (positions[0].x-positions[2].x) * ((float)y-positions[2].y)) * invDET;
        float w3 = 1.0f - w1 - w2;

        if(w1 < 0 || w1 > 1 || w2 <0 || w2 > 1 || w1 + w2 > 1) return false;

        depth = positions[0].z * w1 +  positions[1].z * w2 + positions[2].z * w3;
        uvw = {w1,w2,w3};

        return true;

    }

    /**
     * Check if a triangle is in view
     * @param triangle Clip space triangle
     * @return True if culled
     */
    static bool cull(const Triangle& triangle){
        //Cull completely not visible
        int count = 3;
        for (auto pos : triangle.pos) {
            //if(abs(pos.x) > pos.w || abs(pos.y) > pos.w || abs(pos.z) > pos.w ) count--;
        }
        //todo check this out
        if(count == 0) return true;
        //Cull triangles that have not properly been clipped
        for (auto pos : triangle.pos) {
            //if(abs(pos.z) >= pos.w) return true;
        }

        //Cull backfaces, see https://en.wikipedia.org/wiki/Back-face_culling
        if constexpr (BACKFACE_CULLING){
            if(glm::dot((-glm::vec3(triangle.pos[0])), glm::cross(glm::vec3(triangle.pos[1] - triangle.pos[0]),glm::vec3(triangle.pos[2] - triangle.pos[0]))) >= 0) return true;
            //todo use these auto-generated normals being used in the backface culling for meshes with no vertex normals defined
        }
        return false;
    }


    /**
     * Convert a point on a triangle to barycentric coordinates
     * @param point Point on triangle
     * @param triangle Triangle
     * @return UVW Barycentric coordinates
     * @see https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
     */
    template<int T> static glm::vec3 pointToBarycentric(glm::vec<T,float> point, const glm::vec<T,float> triangle[3])
    {
        //todo optimize
        glm::vec3 v0 = triangle[1] - triangle[0] , v1 =  triangle[2] -  triangle[0], v2 = point -  triangle[0];
        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        return {u,v,w};
    }

    /**
     * Rasterize a triangle to the frame buffer
     * @param clip_tri Triangle after vertex shader and projection
     * @param frame_buffer Frame buffer to write to
     * @param texture Texture to use for colors
     */
    void rasterize(const Triangle& clip_tri,FrameBuffer& frame_buffer, const Texture& texture) const {
        //Cull
        if(cull(clip_tri)) return;
        //Get screen space (x,y,depth)
        glm::vec3 screen_space[3];
        for (int i = 0; i < 3; ++i) {
            screen_space[i] = clip_tri.pos[i] / clip_tri.pos[i].w; //normalize with w
            screen_space[i] = {(screen_space[i].x + 1.0) * ((float)frame_buffer.getWidth()/2.0f),(screen_space[i].y + 1.0) * ((float)frame_buffer.getHeight()/2.0f), (screen_space[i].z+1.0f) * (far_plane - near_plane)/2.0f + near_plane};
        }
        //get bounding box(also clamp to screen bounds)
        glm::int2 box_min = max(min(min(screen_space[0], screen_space[1]),screen_space[2]), {0,0,0});
        glm::int2 box_max = min(max(max(screen_space[0], screen_space[1]),screen_space[2]), {frame_buffer.getWidth()-1,frame_buffer.getHeight()-1, 0});

        //Rasterize
        for (int x = box_min.x; x <= box_max.x; x++) {
            for (int y = box_min.y; y <= box_max.y; y++) {
                glm::vec3 barycentric;
                float depth;
                if(inTriangle(x,y,screen_space,depth,barycentric)) {
                    //todo add fragment shader class here
                    glm::vec2 uv = applyBarycentricPerspective(clip_tri.tex, barycentric, clip_tri.pos);
                    //todo optimize
                    if( texture.isTransparent((int)(uv.x * (float)(texture.getWidth()-1)),(int)(uv.y * (float)(texture.getHeight()-1)))){
                        continue;
                    }
                    Texture::Color texture_color = texture.getPixel((int)(uv.x * (float)(texture.getWidth()-1)),(int)(uv.y * (float)(texture.getHeight()-1)));
                    glm::vec3 color = {(float)texture_color.r,(float)texture_color.g, (float)texture_color.b};
                    frame_buffer.setPixelIfDepth(x,y,{color ,depth});
                }
            }
        }
    }

    /**
     * Move a vertex of a triangle and update(interpolate the attributes accordingly
     * @param triangle Triangle to edit.
     * @param vertex Vertex index. 0,1,2.
     * @param new_position New position of vertex. Must still lie on the triangle.
     */
    static void moveVertex(Triangle& triangle, int vertex, const glm::vec4& new_position){
        glm::vec3 uvw = pointToBarycentric(new_position,triangle.pos);
        triangle.tex[vertex] = applyBarycentric(triangle.tex,uvw);
        //todo normals
        triangle.pos[vertex] = new_position;
    }

    /**
     * Take a line and intersect it with the near clipping plane(0)
     * @param a Start of line
     * @param b End of line
     * At least one part of the line must be one opposite side of the plane
     * @return Intersection point
     */
    [[nodiscard]] glm::vec4 intersectNearPlane(const glm::vec4& a,const glm::vec4& b) const{
        //Get intersection with line xz
        float slope_1 = (a.z-b.z)/(a.x-b.x);
        // z = m(x-x1)+z1, near = m(x-x1)+z1, (near-z1)/m+x1  = x
        float x = (-near_plane-a.z)/slope_1 + a.x;
        //Get intersection with line yz
        float slope_2 = (a.z-b.z)/(a.y-b.y);
        float y = (-near_plane-a.z)/slope_2 + a.y;
        return {x,y,-near_plane,1}; //W is always 1 in view space. Z must be on the plane.
    }


    /**
     * Clip a triangle against the near viewing plane if needed
     * @param view_tri View space triangle
     * @return Resulting clipped triangles
     */
     [[nodiscard]] std::vector<Triangle> clip(const Triangle& view_tri) const {
        //todo optimize and reserve
        std::vector<int> inside;
        std::vector<int> outside;
        for (int i = 0; i < 3; ++i) {
            if(view_tri.pos[i].z < -near_plane){
                inside.push_back(i);
            }else{
                outside.push_back(i);
            }
        }
        if(outside.empty()) return {view_tri}; //All in
        if(inside.size() == 1){ //Move triangle
            Triangle moved_triangle = view_tri;
            int a = inside[0];
            int b = outside[0];
            int c = outside[1];
            moveVertex(moved_triangle,b,intersectNearPlane(view_tri.pos[a],view_tri.pos[b]));
            moveVertex(moved_triangle,c,intersectNearPlane(view_tri.pos[a],view_tri.pos[c]));
            return {moved_triangle};
        }
        if(inside.size() == 2){ //Generate one additional triangle
            int a = inside[0];
            int b = inside[1];
            int c = outside[0];
            glm::vec4 a_intersect = intersectNearPlane(view_tri.pos[a],view_tri.pos[c]);
            glm::vec4 b_intersect = intersectNearPlane(view_tri.pos[b],view_tri.pos[c]);
            Triangle triangle_1 = view_tri;
            Triangle triangle_2 = view_tri;

            moveVertex(triangle_1,c,a_intersect);

            moveVertex(triangle_2,a,a_intersect);
            moveVertex(triangle_2,c,b_intersect);

            return {triangle_1,triangle_2};
        }
        //none visible
        return {};
    }

public:
    /**
     * Create a renderer
     * @param camera Starting camera
     */
    explicit Renderer(const Camera& camera) :camera(camera){
        setCamera(camera);
    }

    /**
     * Set the current camera
     */
    void setCamera(const Camera& new_camera) {
        near_plane = new_camera.getNearPlane();
        far_plane = new_camera.getFarPlane();
        camera = new_camera;
    }

    /**
     * Prepare a frame for rendering
     * @param frame_buffer Frame buffer to render to
     */
    void clearFrame(FrameBuffer& frame_buffer,const glm::vec3& background_color) const {
        for (int x = 0; x < frame_buffer.getWidth(); ++x) {
            for (int y = 0; y < frame_buffer.getHeight(); ++y) {
                frame_buffer.setPixel(x,y,{background_color,far_plane});
            }
        }
    }

    //todo move to gpu backen
    /**
    * Ray march an SDF into the framebuffer
     * @param frame_buffer Frame buffer to render to
     * @param sdf World space SDF
     * @param color Color of SDF
    */
    /*
    void drawSDF(FrameBuffer& frame_buffer, const SDF& sdf, const glm::vec3& color) const {
        for (int x = 0; x < frame_buffer.getWidth(); ++x) {
            for (int y = 0; y < frame_buffer.getHeight(); ++y) {
                //Get clip space -1 to 1 range
                float uvx = 2.0f*((float)x/frame_buffer.getWidth() - 0.5f);
                float uvy = 2.0f*(float)y/frame_buffer.getWidth() - 0.5f;
                //Get ray origin and direction in world space
                //see https://stackoverflow.com/questions/2354821/raycasting-how-to-properly-apply-a-projection-matrix
                glm::mat4 inverse_camera = camera.getInverseMatrix();
                glm::vec3 ray_origin = inverse_camera * glm::vec4{uvx,uvy,-1.0f,1.0f} * near_plane;
                glm::vec3 ray_direction = glm::normalize(inverse_camera * glm::vec4{glm::vec2{uvx,uvy} * (far_plane - near_plane), far_plane + near_plane, far_plane - near_plane});
                //March rays
                //see https://michaelwalczyk.com/blog-ray-marching.html
                float depth = 0;
                const int MAX_STEPS = 40;
                const float MIN_DIST = 0.01;
                for(int i = 0; i < MAX_STEPS; i++){
                    if(depth > far_plane) break;
                    glm::vec3 current_position = ray_origin + ray_direction * depth;
                    float current_distance = sdf(current_position);
                    if(current_distance < MIN_DIST){
                        frame_buffer.setPixelIfDepth(x,y,{color,depth});//todo fix depth
                    }
                    depth += current_distance;
                }
            }
        }
    }
    */




    /**
    * Draw a skinned mesh. Can be called multiple times per frame. Make sure to clear frame every frame.
    * @param frame_buffer Frame buffer to draw to.
    * @param mesh Skinned Mesh to draw. Make sure texture ids match the renderers texture buffer.
    * @param model_transform Transform of mesh.
     * @param bones Bones to pose skinned mesh with their final transforms. Must be meant for the mesh.
     * @param texture Texture to use for rendering
    */
    void drawSkinned(FrameBuffer& frame_buffer, const SkinnedMesh& mesh, const glm::mat4& model_transform, const std::vector<glm::mat4>& bones, const Texture& texture) const {
        assert(mesh.num_bones == (int)bones.size());
        VertexShader vertex_shader{};
        vertex_shader.setCamera(camera);
        vertex_shader.setModelTransform(model_transform);
        for (const SkinnedTriangle& triangle : mesh.tris) {
            Triangle view_tri = vertex_shader.toViewSpaceSkinned(triangle, bones); //Geometry shader
            std::vector<Triangle> clipped_view_tris = clip(view_tri);
            for (const Triangle& clipped_view_tri : clipped_view_tris) {
                Triangle clip_tri = vertex_shader.toClipSpace(clipped_view_tri); //Project
                rasterize(clip_tri,frame_buffer,texture);
            }
        }
    }


    /**
     * Draw a mesh. Can be called multiple times per frame. Make sure to clear frame every frame.
     * @param frame_buffer Frame buffer to draw to.
     * @param mesh Mesh to draw. Make sure texture ids match the renderers texture buffer.
     * @param model_transform Transform of mesh.
     * @param texture Texture to use for rendering
     */
    void draw(FrameBuffer& frame_buffer, const Mesh& mesh, const glm::mat4& model_transform ,const Texture& texture) const {
        VertexShader vertex_shader{};
        vertex_shader.setCamera(camera);
        vertex_shader.setModelTransform(model_transform);
        for (const Triangle& triangle : mesh.tris) {
            Triangle view_tri = vertex_shader.toViewSpace(triangle); //Geometry shader
            std::vector<Triangle> clipped_view_tris = clip(view_tri);
            for (const Triangle& clipped_view_tri : clipped_view_tris) {
                Triangle clip_tri = vertex_shader.toClipSpace(clipped_view_tri); //Project
                rasterize(clip_tri,frame_buffer,texture);
            }
        }
    }
};
