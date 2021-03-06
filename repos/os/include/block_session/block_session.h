/*
 * \brief  Block session interface.
 * \author Stefan Kalkowski
 * \date   2010-07-06
 */

/*
 * Copyright (C) 2010-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BLOCK_SESSION__BLOCK_SESSION_H_
#define _INCLUDE__BLOCK_SESSION__BLOCK_SESSION_H_

#include <os/packet_stream.h>
#include <packet_stream_tx/packet_stream_tx.h>
#include <session/session.h>

namespace Block {

	/**
	 * Sector type for block session
	 */
	typedef Genode::uint64_t sector_t;

	class Packet_descriptor;
	struct Session;
}


/**
 * Represents an block-operation request
 *
 * The data associated with the 'Packet_descriptor' is either
 * the data read from or written to the block indicated by
 * its number.
 */
class Block::Packet_descriptor : public Genode::Packet_descriptor
{
	public:

		enum Opcode    { READ, WRITE, END };
		enum Alignment { PACKET_ALIGNMENT = 11 };

	private:

		Opcode          _op;           /* requested operation */
		sector_t        _block_number; /* requested block number */
		Genode::size_t  _block_count;  /* number of blocks to transfer */
		unsigned        _success :1;   /* indicates success of operation */

	public:

		/**
		 * Constructor
		 */
		Packet_descriptor(Genode::off_t offset=0, Genode::size_t size = 0)
		:
			Genode::Packet_descriptor(offset, size),
			_op(READ), _block_number(0), _block_count(0), _success(false)
		{ }

		/**
		 * Constructor
		 */
		Packet_descriptor(Packet_descriptor p, Opcode op,
		                  sector_t block_number, Genode::size_t block_count = 1)
		:
			Genode::Packet_descriptor(p.offset(), p.size()),
			_op(op), _block_number(block_number),
			_block_count(block_count), _success(false)
		{ }

		Opcode         operation()    const { return _op;           }
		sector_t       block_number() const { return _block_number; }
		Genode::size_t block_count()  const { return _block_count;  }
		bool           succeeded()    const { return _success;      }

		void succeeded(bool b) { _success = b ? 1 : 0; }
};


/*
 * Block session interface
 *
 * A block session corresponds to a block device that can be used to read
 * or store data. Payload is communicated over the packet-stream interface
 * set up between 'Session_client' and 'Session_server'.
 *
 * Even though the methods 'tx' and 'tx_channel' are specific for the client
 * side of the block session interface, they are part of the abstract 'Session'
 * class to enable the client-side use of the block interface via a pointer to
 * the abstract 'Session' class. This way, we can transparently co-locate the
 * packet-stream server with the client in same program.
 */
struct Block::Session : public Genode::Session
{
	enum { TX_QUEUE_SIZE = 256 };


	/**
	 * This class represents supported operations on a block device
	 */
	class Operations
	{
		private:

			unsigned _ops :Packet_descriptor::END; /* bitfield of ops */

		public:

			Operations() : _ops(0) { }

			bool supported(Packet_descriptor::Opcode op) {
				return (_ops & (1 << op)); }

			void set_operation(Packet_descriptor::Opcode op) {
				_ops |= (1 << op); }
	};


	typedef Genode::Packet_stream_policy<Block::Packet_descriptor,
	                                     TX_QUEUE_SIZE, TX_QUEUE_SIZE,
	                                     char> Tx_policy;

	typedef Packet_stream_tx::Channel<Tx_policy> Tx;

	/**
	 * \noapi
	 */
	static const char *service_name() { return "Block"; }

	enum { CAP_QUOTA = 5 };

	virtual ~Session() { }

	/**
	 * Request information about the metrics of the block device
	 *
	 * \param block_count will contain the total number of blocks
	 * \param block_size  will contain the size of one block in bytes
	 * \param ops       supported operations
	 */
	virtual void info(sector_t       *block_count,
	                  Genode::size_t *block_size,
	                  Operations     *ops) = 0;

	/**
	 * Synchronize with block device, like ensuring data to be written
	 */
	virtual void sync() = 0;

	/**
	 * Request packet-transmission channel
	 */
	virtual Tx *tx_channel() { return 0; }

	/**
	 * Request client-side packet-stream interface of tx channel
	 */
	virtual Tx::Source *tx() { return 0; }

	/**
	 * Return capability for packet-transmission channel
	 */
	virtual Genode::Capability<Tx> tx_cap() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_info, void, info, Block::sector_t *,
	           Genode::size_t *, Operations *);
	GENODE_RPC(Rpc_tx_cap, Genode::Capability<Tx>, tx_cap);
	GENODE_RPC(Rpc_sync, void, sync);
	GENODE_RPC_INTERFACE(Rpc_info, Rpc_tx_cap, Rpc_sync);
};

#endif /* _INCLUDE__BLOCK_SESSION__BLOCK_SESSION_H_ */
