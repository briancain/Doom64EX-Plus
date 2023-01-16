#!/bin/bash
gcc -g -DDOOM_UNIX_INSTALL -DUSE_FLUIDSYNTH `pkg-config --static --cflags sdl2` -Ibackend/glad_3.2/include  backend/glad_3.2/src/glad.c am_draw.c am_map.c con_console.c con_cvar.c d_devstat.c d_iwad.c d_main.c d_net.c deh_ammo.c deh_doom.c deh_frame.c deh_io.c deh_main.c deh_mapping.c deh_misc.c deh_ptr.c deh_str.c deh_thing.c deh_weapon.c f_finale.c g_actions.c g_demo.c g_game.c g_settings.c gl_draw.c gl_main.c gl_pixmap.c gl_shader.c gl_texture.c gl_utils.c i_audio.c i_main.c i_png.c i_sdlinput.c i_system.c i_video.c i_w3swrapper.c in_stuff.c info.c m_cheat.c m_fixed.c m_keys.c m_menu.c m_misc.c m_password.c m_random.c m_shift.c md5.c net_client.c net_common.c net_dedicated.c net_io.c net_loop.c net_packet.c net_query.c net_server.c net_structure.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_macros.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c psnprntf.c r_bsp.c r_clipper.c r_drawlist.c r_lights.c r_main.c r_scene.c r_sky.c r_things.c r_wipe.c s_sound.c sc_main.c sha1.c st_stuff.c tables.c w_file.c w_merge.c w_wad.c wi_stuff.c z_zone.c -o DOOM64EX+ -I/usr/local/include/SDL2 `pkg-config --static --libs sdl2` `pkg-config --static --libs libpng` `pkg-config --static --libs fluidsynth` `pkg-config --static --libs gl` `pkg-config --static --libs glu` `pkg-config --static --libs SDL2_net` -lm

