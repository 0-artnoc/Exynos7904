/*
 *  Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FSCRYPTO_SDP_DEK_H
#define _FSCRYPTO_SDP_DEK_H

#ifdef CONFIG_EXT4CRYPT_SDP
struct sdp_info {
	u32 sdp_flags;
	u32 engine_id;
	dek_t sdp_dek;
	spinlock_t sdp_flag_lock;
};

// Essential material for set_sensitive operation
typedef struct _sdp_ess_material {
    struct inode *inode;
    /* EXT4CRYPT-dedicated */
    struct ext4_encryption_key key;
} sdp_ess_material;

int fscrypt_sdp_set_sdp_policy(struct inode *inode, int engine_id);
int fscrypt_sdp_set_sensitive(struct inode *inode, int engine_id, /* EXT4CRYPT-dedicated */struct ext4_encryption_key *key);
int fscrypt_sdp_set_protected(struct inode *inode, int engine_id);
int fscrypt_sdp_add_chamber_directory(int engine_id, struct inode *inode);
int fscrypt_sdp_remove_chamber_directory(struct inode *inode);
int fscrypt_sdp_get_engine_id(struct inode *inode);
int fscrypt_sdp_inherit_context(struct inode *parent, struct inode *child, /* EXT4CRYPT-dedicated */struct ext4_encryption_context *ctx);
void fscrypt_sdp_finalize_tasks(struct inode *inode, u8 *raw_key, int key_len);
struct sdp_info *fscrypt_sdp_alloc_sdp_info(void);
void fscrypt_sdp_put_sdp_info(struct sdp_info *ci_sdp_info);
bool fscrypt_sdp_init_sdp_info_cachep(void);
void fscrypt_sdp_release_sdp_info_cachep(void);
int fscrypt_sdp_derive_dek(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info,
						unsigned char *derived_key,
						unsigned int derived_keysize);
int fscrypt_sdp_derive_uninitialized_dek(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info,
						unsigned char *derived_key,
						unsigned int derived_keysize);
int fscrypt_sdp_derive_fekey(struct inode *inode,
		/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info,
		/* EXT4CRYPT-dedicated */struct ext4_encryption_key *fek);
int fscrypt_sdp_derive_fek(struct inode *inode,
		/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info,
						unsigned char *fek, unsigned int fek_len);
int fscrypt_sdp_store_fek(struct inode *inode,
		/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info,
						unsigned char *fek, unsigned int fek_len);
int fscrypt_sdp_update_sdp_info(struct inode *inode,
		/* EXT4CRYPT-dedicated */const struct ext4_encryption_context *ctx,
		/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
int fscrypt_sdp_is_classified(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
int fscrypt_sdp_is_uninitialized(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
int fscrypt_sdp_is_native(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
int fscrypt_sdp_is_sensitive(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
int fscrypt_sdp_is_to_sensitive(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
void fscrypt_sdp_update_conv_status(/* EXT4CRYPT-dedicated */struct ext4_crypt_info *crypt_info);
#endif

#endif	/* _FSCRYPTO_SDP_DEK_H */
