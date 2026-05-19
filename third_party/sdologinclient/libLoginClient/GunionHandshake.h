#pragma once

///////////////////////////////////////////////////////////////////////////////
// GunionHandshake.h
//
// 说明：
//   Gunion-WeGame 业务层“安全握手”状态机
//
//   本模块负责：
//     - 控制 Gunion-WeGame 握手流程（GetPublicKey → NegotiateKey）
//     - 保存握手中间态（publicKey / sessionKey 等）
//     - 不直接发送 HTTP 请求
//
//   不负责：
//     - 网络发送（由 LoginClient / HttpThread 负责）
//     - 协议解析（ret / kv / JSON / map 等）
//     - libcurl / 线程 / TLS
//
//   设计原则：
//     - 只描述“业务流程 / 状态机”
//     - 通过 LoginClient 触发请求
//     - 通过“语义级回调”接收结果
///////////////////////////////////////////////////////////////////////////////

#include <string>

// 前向声明，避免头文件互相 include
class LoginClient;

class GunionHandshake
{
public:
	/**
	 * @brief 构造函数
	 *
	 * @param owner 所属 LoginClient（用于发请求、取上下文）
	 *
	 * 注意：
	 *   - GunionHandshake 不拥有 LoginClient
	 *   - 生命周期由 LoginClient 管理
	 */
	explicit GunionHandshake(LoginClient* owner);

	/**
	 * @brief 启动 Gunion-WeGame 握手流程（异步状态机入口）
	 *
	 * 流程说明：
	 *   1. 若尚未获取 PublicKey：
	 *        → 通过 LoginClient 发起 GetPublicKey 请求（异步）
	 *   2. 若已具备 PublicKey：
	 *        → 通过 LoginClient 发起 NegotiateKey 请求（异步）
	 *
	 * 设计说明：
	 *   - 本函数仅负责“推进一步流程”
	 *   - 不保证流程立即完成
	 *   - 实际成功 / 失败结果由 LoginClient
	 *     通过语义回调函数驱动
	 *
	 * 注意：
	 *   - 同一时刻只会触发一个网络请求
	 *   - 不会在此函数中连续触发多个请求
	 */
	bool Start();

	///////////////////////////////////////////////////////////////////////////////
	// ↓↓↓ 语义级回调接口（由 LoginClient 调用） ↓↓↓
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief GetPublicKey 成功回调
	 *
	 * @param publicKey 服务端下发的公钥
	 *
	 * 行为：
	 *   - 保存 publicKey
	 *   - 推进状态机，继续发起 NegotiateKey 请求
	 *
	 * 说明：
	 *   - 本函数不关心网络层 / 协议层细节
	 *   - 只表示“PublicKey 阶段成功”
	 */
	void OnPublicKeyReady(const std::string& publicKey);

	/**
	 * @brief GetPublicKey 失败回调
	 *
	 * @param reason 失败原因（由 LoginClient 传递）
	 *
	 * 行为：
	 *   - 结束握手流程
	 *   - 标记失败
	 */
	void OnPublicKeyFailed(int reason);

	/**
	 * @brief NegotiateKey 成功回调
	 *
	 * 行为：
	 *   - 标记握手流程成功
	 *
	 * 说明：
	 *   - 后续如需保存 sessionKey / randKey / token
	 *     可在此接口中扩展参数
	 */
	void OnHandShakeSuccess();

	/**
	 * @brief NegotiateKey 失败回调
	 *
	 * @param reason 失败原因（由 LoginClient 传递）
	 *
	 * 行为：
	 *   - 结束握手流程
	 *   - 标记失败
	 */
	void OnHandShakeFailed(int reason);

	///////////////////////////////////////////////////////////////////////////////
	// ↓↓↓ 状态查询接口 ↓↓↓
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 判断握手流程是否已结束
	 *
	 * @return true 已结束（成功或失败）
	 */
	bool IsFinished() const;

	/**
	 * @brief 判断本次握手流程是否成功
	 *
	 * @return
	 *   - true  ：本次握手流程已成功完成
	 *   - false ：本次握手流程尚未完成，或已失败
	 *
	 * 注意：
	 *   - 仅当 IsFinished() == true 时，该返回值才表示最终结果
	 *   - 若 IsFinished() == false，则表示握手流程仍在进行中
	 */
	bool IsSuccess() const;

	/**
	 * @brief 重置握手状态，用于重新登录 / 重新发起一次新的握手流程
	 *
	 * 行为说明：
	 *   - 清空上一次握手过程中的中间状态（如 publicKey）
	 *   - 重置内部流程标志（started / finished / success）
	 *   - 使当前 GunionHandshake 实例回到“未启动”初始状态
	 *
	 * 使用场景：
	 *   - 用户重新登录
	 *   - 上一次握手失败后需要重试
	 *   - 业务上明确要求重新建立 Gunion 会话
	 *
	 * 注意：
	 *   - 调用 Reset() 后，需要重新调用 Start()
	 *     或 EnsureGunionReady() 才会再次启动握手流程
	 *   - Reset() 不会触发任何网络请求
	 */
	void Reset();


private:
	/**
	 * @brief 是否需要先请求 publicKey
	 *
	 * @return
	 *   - true  ：尚未获取 publicKey，需要调用 GetPublicKey
	 *   - false ：已具备 publicKey，可直接协商
	 */
	bool NeedGetPublicKey() const;

private:
	LoginClient* m_owner;     ///< 所属 LoginClient（非拥有）
	std::string m_publicKey; ///< 服务端下发的公钥

	bool m_finished;          ///< 握手流程是否结束
	bool m_success;           ///< 握手是否成功

	bool m_started;           ///< 是否已经启动过握手流程
};
