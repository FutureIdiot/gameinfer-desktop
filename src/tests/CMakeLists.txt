find_package(SndFile CONFIG REQUIRED)

add_executable(TestGameInferQueue
        gameinfer_queue_test.cpp
        ../apps/GameInfer/gui/InferenceQueueController.cpp
        ../apps/GameInfer/gui/InferenceQueueController.h
)
set_target_properties(TestGameInferQueue PROPERTIES AUTOMOC ON)
target_include_directories(TestGameInferQueue PRIVATE ../apps/GameInfer/gui)
target_link_libraries(TestGameInferQueue PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Concurrent
)

add_executable(TestGameInferWorkspace
        gameinfer_workspace_test.cpp
        ../apps/GameInfer/workspace/WorkspaceManager.cpp
        ../apps/GameInfer/workspace/WorkspaceManager.h
        ../apps/GameInfer/utils/AppPaths.cpp
        ../apps/GameInfer/utils/AppPaths.h
)
target_include_directories(TestGameInferWorkspace PRIVATE
        ../apps/GameInfer/workspace
        ../apps/GameInfer
)
target_link_libraries(TestGameInferWorkspace PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)

add_executable(TestGameInferManualSlice
        gameinfer_manual_slice_test.cpp
        ../apps/GameInfer/gui/ManualSliceDialog.cpp
        ../apps/GameInfer/gui/ManualSliceDialog.h
)
set_target_properties(TestGameInferManualSlice PROPERTIES AUTOMOC ON)
target_include_directories(TestGameInferManualSlice PRIVATE ../apps/GameInfer/gui)
target_link_libraries(TestGameInferManualSlice PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        SndFile::sndfile
        audio-util
        SDLPlayback
)
