if not exist rage goto wronglocation

if "%1"=="" goto nolabel

p4 sync rage/...#0
rd /S /Q rage
del rage.zip
p4 sync rage/...@%1
p4 sync rage/3rdParty/...#0
p4 sync rage/qa/...#0
p4 sync rage/doc/...#0
p4 sync rage/base/doc/...#0
p4 sync rage/base/qa/...#0
p4 sync rage/base/samples/sample_atl/...#0
p4 sync rage/base/samples/sample_audio/...#0
p4 sync rage/base/samples/sample_bank/...#0
p4 sync rage/base/samples/sample_blendshapes/...#0
p4 sync rage/base/samples/sample_cranimation/...#0
p4 sync rage/base/samples/sample_curve/...#0
p4 sync rage/base/samples/sample_flash/...#0
p4 sync rage/base/samples/sample_games/...#0
p4 sync rage/base/samples/sample_gizmo/...#0
p4 sync rage/base/samples/sample_input/...#0
p4 sync rage/base/samples/sample_mesh/...#0
p4 sync rage/base/samples/sample_net/...#0
p4 sync rage/base/samples/sample_net_physics/...#0
p4 sync rage/base/samples/sample_paging/...#0
p4 sync rage/base/samples/sample_parser/...#0
p4 sync rage/base/samples/sample_profile/...#0
p4 sync rage/base/samples/sample_rline/...#0
p4 sync rage/base/samples/sample_rmocclude/...#0
p4 sync rage/base/samples/sample_string/...#0
p4 sync rage/base/samples/sample_system/...#0
p4 sync rage/base/samples/sample_world/...#0
p4 sync rage/base/samples/sample_xmldata/...#0
p4 sync rage/base/src/audiodriver/...#0
p4 sync rage/base/src/audiomediaplayer/...#0
p4 sync rage/base/src/mesh2/...#0
p4 sync rage/base/src/net/...#0
p4 sync rage/base/src/phinertia/...#0
p4 sync rage/base/src/ptxcore/...#0
p4 sync rage/base/src/rline/...#0
p4 sync rage/base/src/rpc/...#0
p4 sync rage/base/src/world/...#0
p4 sync rage/base/tools/...#0
p4 sync rage/framework/...#0
p4 sync rage/migrate/...#0
p4 sync rage/rad/...#0
p4 sync rage/speedtree/...#0
p4 sync rage/suite/doc/...#0
p4 sync rage/suite/qa/...#0
p4 sync rage/suite/samples/sample_actor/...#0
p4 sync rage/suite/samples/sample_ai/...#0
p4 sync rage/suite/samples/sample_aicover/...#0
p4 sync rage/suite/samples/sample_aiveh/...#0
p4 sync rage/suite/samples/sample_aiworld/...#0
p4 sync rage/suite/samples/sample_bink/...#0
p4 sync rage/suite/samples/sample_camclient/...#0
p4 sync rage/suite/samples/sample_charactercloth/...#0
p4 sync rage/suite/samples/sample_cloth/...#0
p4 sync rage/suite/samples/sample_crowd/...#0
p4 sync rage/suite/samples/sample_driving/...#0
p4 sync rage/suite/samples/sample_event/...#0
p4 sync rage/suite/samples/sample_geo/...#0
p4 sync rage/suite/samples/sample_grass/...#0
p4 sync rage/suite/samples/sample_lighting/...#0
p4 sync rage/suite/samples/sample_motionfamilies/...#0
p4 sync rage/suite/samples/sample_motiontree/...#0
p4 sync rage/suite/samples/sample_netroute/...#0
p4 sync rage/suite/samples/sample_netsynch/...#0
p4 sync rage/suite/samples/sample_ponytail/...#0
p4 sync rage/suite/samples/sample_rope/...#0
p4 sync rage/suite/samples/sample_rmlighting/...#0
p4 sync rage/suite/samples/sample_rmptfx/...#0
p4 sync rage/suite/samples/sample_rmworld/...#0
p4 sync rage/suite/samples/sample_styletransfer/...#0
p4 sync rage/suite/samples/sample_terrainlighting/...#0
p4 sync rage/suite/samples/sample_trigger/...#0
p4 sync rage/suite/samples/sample_veh/...#0
p4 sync rage/suite/samples/sample_wave/...#0
p4 sync rage/suite/samples/sample_wilderness/...#0
p4 sync rage/suite/3rdparty/...#0
p4 sync rage/suite/src/actiontree/...#0
p4 sync rage/suite/src/aicore/...#0
p4 sync rage/suite/src/aicover/...#0
p4 sync rage/suite/src/ainav/...#0
p4 sync rage/suite/src/ainavobj/...#0
p4 sync rage/suite/src/ainavigation/...#0
p4 sync rage/suite/src/ainavrmworld/...#0
p4 sync rage/suite/src/aiveh/...#0
p4 sync rage/suite/src/bink/...#0
p4 sync rage/suite/src/camclient/...#0
p4 sync rage/suite/src/cammach/...#0
p4 sync rage/suite/src/crowd/...#0
p4 sync rage/suite/src/demesh/...#0
p4 sync rage/suite/src/geoload/...#0
p4 sync rage/suite/src/grass/...#0
p4 sync rage/suite/src/grrope/...#0
p4 sync rage/suite/src/mfcore/...#0
p4 sync rage/suite/src/mffactory/...#0
p4 sync rage/suite/src/mfruntime/...#0
p4 sync rage/suite/src/netroute/...#0
p4 sync rage/suite/src/netsynch/...#0
p4 sync rage/suite/src/rageLevel/...#0
p4 sync rage/suite/src/rmptx/...#0
p4 sync rage/suite/src/shadercontrollers/...#0
p4 sync rage/suite/src/snet/...#0
p4 sync rage/suite/src/snu/...#0
p4 sync rage/suite/src/terrainlighting/...#0
p4 sync rage/suite/src/trigger/...#0
p4 sync rage/suite/src/triggeractor/...#0
p4 sync rage/suite/src/triggercore/...#0
p4 sync rage/suite/src/vehbase/...#0
p4 sync rage/suite/src/vehdyna/...#0
p4 sync rage/suite/src/wildshaders/...#0
p4 sync rage/suite/tools/lib/...#0
p4 sync rage/suite/tools/ainavigationbuild/...#0
p4 sync rage/suite/tools/ainavtools/...#0
p4 sync rage/suite/tools/aisimplenavbuild/...#0
p4 sync rage/suite/tools/airc/...#0
p4 sync rage/suite/tools/airesourcecompiler/...#0
p4 sync rage/suite/tools/CamMachIdle/...#0
p4 sync rage/suite/tools/demeshtools/...#0
p4 sync rage/suite/tools/drvtool/...#0
p4 sync rage/suite/tools/makeworld/...#0
p4 sync rage/suite/tools/mftools/...#0
p4 sync rage/suite/tools/PyCamMach/...#0
p4 sync rage/suite/tools/rageGenerateIntervalMaps/...#0
p4 sync rage/suite/tools/srorc/...#0
p4 sync rage/suite/tools/styletools/...#0
p4 sync rage/suite/tools/viewshot/...#0
p4 sync rage/tools/...#0
p4 sync rage/top/output/...#0
p4 sync rage/top/screens/...#0
p4 sync rage/top/*.hta#0
p4 sync rage/.../*.chm#0
p4 sync rage/.../*.doc#0
p4 sync rage/.../*.HxC#0
p4 sync rage/.../*.HxF#0
p4 sync rage/.../*.HxI#0
p4 sync rage/.../*.HxK#0
p4 sync rage/.../*.HxS#0
p4 sync rage/.../*.HxT#0
p4 sync rage/.../*.dox#0
p4 sync rage/.../*.ppt#0
xcopy /I /Y /E t:\rage\assets\physics rage\assets\physics
p4 sync rage\assets\physics\blt
p4 sync rage\assets\physics\mc4city
xcopy /I /Y /E t:\rage\assets\naturalmotion rage\assets\naturalmotion
xcopy /I /Y /E t:\rage\assets\sample_saloon rage\assets\sample_saloon
xcopy /I /Y /E t:\rage\assets\tune\shaders rage\assets\tune\shaders
zip -r -9 rage.zip rage
pause

goto eof

:wronglocation

REM To use, copy into your soft directory (the same level as your rage directory) and run from there.

exit/B

:nolabel

REM Usage: %0 [p4 label]

exit/B

:eof
