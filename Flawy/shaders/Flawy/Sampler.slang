#pragma once

#define FLAWY_SAMPLER2D(_name, _stage) \
    [[vk::binding(2 + _stage, 0)]] Texture2D _name : register(t##_stage); \
    [[vk::binding(18 + _stage, 0)]] SamplerState _name##Sampler : register(s##_stage)

#define FLAWY_SAMPLE2D(_name, _uv) _name.Sample(_name##Sampler, _uv)
