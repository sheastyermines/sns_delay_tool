#!/bin/bash

ach rm state_raw;
ach rm state_delayed;
ach rm ref_raw;
ach rm ref_delayed;

ach mk state_raw;
ach mk state_delayed;
ach mk ref_raw;
ach mk ref_delayed;

export SNS_SCENE_PLUGIN=~/Research/amino/demo/urdf/baxter/.libs/libamino_baxter.so;
export SNS_SCENE_NAME=baxter;

export SNS_CHANNEL_MAP_state_one="one_s0,one_s1,one_e0,one_e1,one_w0,one_w1,one_w2";
export SNS_CHANNEL_MAP_state_two="two_s0,two_s1,two_e0,two_e1,two_w0,two_w1,two_w2";

export SNS_CHANNEL_MAP_ref_one="$SNS_CHANNEL_MAP_state_one";
export SNS_CHANNEL_MAP_ref_two="$SNS_CHANNEL_MAP_state_two";


