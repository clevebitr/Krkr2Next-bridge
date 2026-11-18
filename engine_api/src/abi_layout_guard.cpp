/**
 * @file abi_layout_guard.cpp
 * @brief engine_api.h 的取值与内存布局守卫（编译期断言，无测试框架依赖）
 *
 * 原为 tests/unit-tests/engine_api/abi-layout.cpp（Catch2 运行期测试）。
 * 随 tests/ 清理迁入 bridge：ABI 是 bridge 的契约，用 static_assert 表达后
 * **每次构建都会检查**，比"记得跑测试"更可靠，也不需要 Catch2。
 *
 * C ABI 是对外壳的稳定契约。Kotlin 侧 dev.kirinext.core.NativeEngine 与
 * EngineSession.InputEvent **硬编码了下面的枚举取值**（JNI 只传裸 int，编译器
 * 无法替我们检查）。改了 C 头而没同步 Kotlin，不会编译报错，只会在运行时
 * 表现为"事件发出去没反应"或"状态判断永远不成立"。
 *
 * 本测试把这些取值钉住：任何改动都会在这里失败，逼开发者去同步外壳。
 *
 * 只依赖 engine_api.h，不链接引擎——配置快、无 vcpkg 依赖。
 */



#include <cstddef>
#include <type_traits>

#include "engine_api.h"

// ---------------------------------------------------------------------------
// 版本
// ---------------------------------------------------------------------------

    // 改这个值意味着对外壳的破坏性变更：外壳会用它做兼容性判断
    static_assert(ENGINE_API_VERSION == 0x01000000u);

    // 版本号编码约定：major(8) | minor(8) | patch(16)
    static_assert(ENGINE_API_MAKE_VERSION(1, 0, 0) == 0x01000000u);
    static_assert(ENGINE_API_MAKE_VERSION(0, 1, 0) == 0x00010000u);

// ---------------------------------------------------------------------------
// 枚举取值
// ---------------------------------------------------------------------------

    static_assert(ENGINE_RESULT_OK == 0);
    static_assert(ENGINE_RESULT_INVALID_ARGUMENT == -1);
    static_assert(ENGINE_RESULT_INVALID_STATE == -2);
    static_assert(ENGINE_RESULT_NOT_SUPPORTED == -3);
    static_assert(ENGINE_RESULT_IO_ERROR == -4);
    static_assert(ENGINE_RESULT_INTERNAL_ERROR == -5);
    static_assert(ENGINE_RESULT_GAME_TERMINATED == -6);
    static_assert(ENGINE_RESULT_STARTUP_PENDING == -7);
    static_assert(ENGINE_RESULT_WINDOW_CLOSED == -8);
    static_assert(ENGINE_RESULT_WINDOW_CLOSE_REQUESTED == -9);

    // 约定：0 为成功，负值为错误。外壳依赖这一点判断成败。
    // 例外：ENGINE_RESULT_GAME_TERMINATED、ENGINE_RESULT_STARTUP_PENDING 与
    // ENGINE_RESULT_WINDOW_CLOSE_REQUESTED 也是负值，但语义分别是"游戏自己要求
    // 退出"、"游戏仍在启动"和"游戏请求关窗、等宿主确认"，都不是失败 ——
    // 外壳必须把它们排除在失败计数之外。
    static_assert(ENGINE_RESULT_OK == 0);

    // 对应 Kotlin 侧 EngineSession.InputEvent 的常量。
    // 取值即引擎内部 kEngineInput* 的分派依据，改动会导致事件被丢弃。
    static_assert(ENGINE_INPUT_EVENT_POINTER_DOWN == 1);
    static_assert(ENGINE_INPUT_EVENT_POINTER_MOVE == 2);
    static_assert(ENGINE_INPUT_EVENT_POINTER_UP == 3);
    static_assert(ENGINE_INPUT_EVENT_POINTER_SCROLL == 4);
    static_assert(ENGINE_INPUT_EVENT_KEY_DOWN == 5);
    static_assert(ENGINE_INPUT_EVENT_KEY_UP == 6);
    static_assert(ENGINE_INPUT_EVENT_TEXT_INPUT == 7);
    static_assert(ENGINE_INPUT_EVENT_BACK == 8);

    // 对应 Kotlin 侧 NativeEngine.STARTUP_*。外壳轮询这个值做进度覆盖层，
    // 并把 SUCCEEDED 当作"首帧已出"的判据。
    static_assert(ENGINE_STARTUP_STATE_IDLE == 0);
    static_assert(ENGINE_STARTUP_STATE_RUNNING == 1);
    static_assert(ENGINE_STARTUP_STATE_SUCCEEDED == 2);
    static_assert(ENGINE_STARTUP_STATE_FAILED == 3);

// ---------------------------------------------------------------------------
// 结构体布局
//
// 以下断言限定 LP64（Android arm64-v8a 与 Linux x86_64 都是）。本项目只构建
// 这两个目标；若将来加入 32 位 ABI，这里需要按平台分支。
// ---------------------------------------------------------------------------

    static_assert(sizeof(void *) == 8, "ABI 守卫只覆盖 LP64（arm64-v8a / x86_64）"); // 本测试仅对 LP64 有效

    static_assert(sizeof(engine_input_event_t) == 104);

    // 字段顺序即 ABI。JNI 包装虽按字段逐个传参，但 Dart 时代的 FFI 调用方与
    // 引擎内部 struct_size 校验都依赖它是这个顺序。
    static_assert(offsetof(engine_input_event_t, struct_size) == 0);
    static_assert(offsetof(engine_input_event_t, type) == 4);
    static_assert(offsetof(engine_input_event_t, timestamp_micros) == 8);
    static_assert(offsetof(engine_input_event_t, x) == 16);
    static_assert(offsetof(engine_input_event_t, y) == 24);
    static_assert(offsetof(engine_input_event_t, delta_x) == 32);
    static_assert(offsetof(engine_input_event_t, delta_y) == 40);
    static_assert(offsetof(engine_input_event_t, pointer_id) == 48);
    static_assert(offsetof(engine_input_event_t, button) == 52);
    static_assert(offsetof(engine_input_event_t, key_code) == 56);
    static_assert(offsetof(engine_input_event_t, modifiers) == 60);
    static_assert(offsetof(engine_input_event_t, unicode_codepoint) == 64);
    static_assert(offsetof(engine_input_event_t, reserved_u32) == 68);
    static_assert(offsetof(engine_input_event_t, reserved_u64) == 72);
    static_assert(offsetof(engine_input_event_t, reserved_ptr) == 88);

    static_assert(sizeof(void *) == 8, "ABI 守卫只覆盖 LP64（arm64-v8a / x86_64）");

    // struct_size 必须是首个字段：引擎靠它做前向兼容判断
    static_assert(offsetof(engine_create_desc_t, struct_size) == 0);
    static_assert(offsetof(engine_create_desc_t, api_version) == 4);
    static_assert(offsetof(engine_create_desc_t, writable_path_utf8) == 8);
    static_assert(offsetof(engine_create_desc_t, cache_path_utf8) == 16);
    static_assert(offsetof(engine_create_desc_t, user_data) == 24);
    static_assert(offsetof(engine_create_desc_t, reserved_u64) == 32);
    static_assert(offsetof(engine_create_desc_t, reserved_ptr) == 64);

    static_assert(sizeof(void *) == 8, "ABI 守卫只覆盖 LP64（arm64-v8a / x86_64）");

    static_assert(offsetof(engine_option_t, key_utf8) == 0);
    static_assert(offsetof(engine_option_t, value_utf8) == 8);
    static_assert(offsetof(engine_option_t, reserved_u64) == 16);
    static_assert(offsetof(engine_option_t, reserved_ptr) == 32);

    // 约定：所有"调用方先初始化"的结构体都以 uint32_t struct_size 打头，
    // 引擎据此区分新旧调用方。此约定一旦破坏，旧外壳会在运行时静默错位。
    static_assert(std::is_same<decltype(engine_input_event_t::struct_size),
                                uint32_t>::value);
    static_assert(std::is_same<decltype(engine_create_desc_t::struct_size),
                                uint32_t>::value);
    static_assert(
        std::is_same<decltype(engine_option_t::key_utf8), const char *>::value);
