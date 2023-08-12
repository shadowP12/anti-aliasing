#pragma once

class Renderer;

class BasePass
{
public:
    BasePass(Renderer* renderer);

    void render();

private:
    Renderer* _renderer;
};