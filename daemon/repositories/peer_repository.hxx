#include <cstdint>
#include <memory>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <peer.hxx>
#include <sstream>
#include <stdexcept>
#include <wgallowedip.hpp>

#include "icrudrepository.h"
#include "peer-odb.hxx"
#include "wgforge.h"

/*

namespace WgForge {

using Key = int32_t;

class PeerRepository : public ICrudRepositoryAsync<Peer, Key> {
public:
  PeerRepository(std::shared_ptr<odb::database> context);

  virtual std::future<std::optional<Peer>> getAsync(const Key& key) const override;
  virtual std::future<std::optional<std::vector<Peer>>> getAllAsync() const override;
  virtual std::future<void> createAsync(const Peer& peer) override;
  virtual std::future<void> updateAsync(const Peer& peer) override;
  virtual std::future<void> softDeleteAsync(const Key& key) override;
  virtual std::future<void> hardDeleteAsync(const Key& key) override;

private:
  std::shared_ptr<odb::database> context_;
};

inline PeerRepository::PeerRepository(std::shared_ptr<odb::database> context) {
  if (context == nullptr)
	throw std::invalid_argument("Null ptr to database context");
  context_ = std::move(context);
}

inline std::future<std::optional<Peer>> PeerRepository::getAsync(const Key& key) const {
  return std::async(std::launch::async, [this, key] -> std::optional<Peer> {
	odb::transaction t(context_->begin());
	auto result = context_->query<Peer>(odb::query<Peer>::id == key);
	auto it = result.begin();
	if (it == result.end())
	  return std::nullopt;
	Peer p = *it;
	t.commit();
	return p;
  });
}

inline std::future<std::optional<std::vector<Peer>>> PeerRepository::getAllAsync() const {
  return std::async(std::launch::async, [this] -> std::optional<std::vector<Peer>> {
	odb::transaction t(context_->begin());
	auto result = context_->query<Peer>();
	auto it = result.begin();
	if (it == result.end())
	  return std::nullopt;
	std::vector<Peer> v(result.begin(), result.end());
	t.commit();
	return v;
  });
}

inline std::future<void> PeerRepository::createAsync(const Peer& peer) {
  if (!WgPublicKey::validateB64StringKey(peer.public_key))
	throw std::invalid_argument("Invalid public key");
  if (peer.preshared_key && !WgPresharedKey::validateB64StringKey(peer.preshared_key.value()))
	throw std::invalid_argument("Invalid preshared key");
  if (!WgAllowedIP::validate(peer.tunnel_address))
	throw std::invalid_argument("Invalid tunnel address");

  Peer copy = peer;

  return std::async(std::launch::async, [this, copy = std::move(peer)]() mutable -> void {
	odb::transaction t(context_->begin());

	copy.id = 0;
	copy.status = Peer::Status::Active;
	copy.created_at = getCurrentUtcTimestamp();
	copy.updated_at = std::nullopt;
	copy.deleted_at = std::nullopt;

	context_->persist(copy);
	t.commit();
  });
}

inline std::future<void> PeerRepository::updateAsync(const Peer& peer) {
  if (!WgAllowedIP::validate(peer.tunnel_address))
	throw std::invalid_argument("Invalid tunnel address");

  Peer copy = peer;

  return std::async(std::launch::async, [this, copy = std::move(copy)]() mutable -> void {
	odb::transaction t(context_->begin());

	copy.updated_at = getCurrentUtcTimestamp();

	context_->update(copy);
	t.commit();
  });
}

inline std::future<void> PeerRepository::softDeleteAsync(const Key& key) {
  return std::async(std::launch::async, [this, key] -> void {
	odb::transaction t(context_->begin());

	auto result = context_->query<Peer>(odb::query<Peer>::id == key);
	if (result.begin() == result.end())
	  throw std::runtime_error("Peer not found by provided key");

	auto peer = *result.begin();
	peer.status = Peer::Status::Suspended;
	peer.updated_at = getCurrentUtcTimestamp();
	peer.deleted_at = getCurrentUtcTimestamp();

	context_->persist(peer);
	t.commit();
  });
}

inline std::future<void> PeerRepository::hardDeleteAsync(const Key& key) {
}



}  // namespace WgForge

*/
