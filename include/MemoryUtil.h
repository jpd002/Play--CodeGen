#pragma once

#include <cstddef>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace CodeGen
{

	// ============================================================================
	// API PUBLIQUE
	// ============================================================================

	/// Alloue de la mémoire exécutable
	/// @param size Taille en bytes (sera alignée sur page)
	/// @return Pointeur vers mémoire exécutable, ou nullptr si échec
	void* AllocateExecutableMemory(size_t size);

	/// Libère de la mémoire exécutable
	/// @param ptr Pointeur retourné par AllocateExecutableMemory()
	/// @param size Taille originale
	void FreeExecutableMemory(void* ptr, size_t size);

#if TARGET_OS_IOS

	// ============================================================================
	// TYPES ET CONFIGURATION (iOS uniquement)
	// ============================================================================

	/// Types de JIT supportés
	enum class JitType
	{
		Legacy,    ///< iOS < 26 : Toggle W^X avec mprotect()
		LuckNoTXM, ///< iOS 26+ sans TXM : Miroirs RW/RX par allocation
		LuckTXM    ///< iOS 26+ avec TXM : Région 512 MB pré-allouée
	};

	/// Configure le type de JIT à utiliser
	/// @note DOIT être appelé avant toute allocation
	/// @param type Type de JIT (détecté par Play!)
	void SetJitType(JitType type);

	// ============================================================================
	// GESTION WRITE/EXECUTE (Legacy uniquement)
	// ============================================================================

	/// Active écriture, désactive exécution (Legacy)
	void JITPageWriteEnableExecuteDisable(void* ptr);

	/// Désactive écriture, active exécution (Legacy)
	void JITPageWriteDisableExecuteEnable(void* ptr);

	/// RAII wrapper pour accès en écriture sécurisé
	struct ScopedJITPageWriteAndNoExecute
	{
		void* ptr;

		ScopedJITPageWriteAndNoExecute(void* region)
		    : ptr(region)
		{
			JITPageWriteEnableExecuteDisable(ptr);
		}

		~ScopedJITPageWriteAndNoExecute()
		{
			JITPageWriteDisableExecuteEnable(ptr);
		}
	};

	// ============================================================================
	// GESTION RÉGIONS RW/RX (LuckNoTXM et LuckTXM)
	// ============================================================================

	/// Pré-alloue la région exécutable (LuckTXM uniquement)
	/// @note Doit être appelé une fois au démarrage si mode LuckTXM
	void AllocateExecutableMemoryRegion();

	/// Obtient le décalage vers région writable
	/// @param rx_ptr Pointeur exécutable
	/// @param size Taille de la région
	/// @return Décalage à ajouter pour obtenir pointeur writable
	ptrdiff_t AllocateWritableRegionAndGetDiff(void* rx_ptr, size_t size);

	/// Libère la région writable (LuckNoTXM uniquement)
	/// @param rx_ptr Pointeur exécutable
	/// @param size Taille
	/// @param diff Décalage retourné par AllocateWritableRegionAndGetDiff()
	void FreeWritableRegion(void* rx_ptr, size_t size, ptrdiff_t diff);

	// ============================================================================
	// DÉCLARATIONS INTERNES (ne pas utiliser directement)
	// ============================================================================

	// LuckTXM
	void* AllocateExecutableMemory_LuckTXM(size_t size);
	void FreeExecutableMemory_LuckTXM(void* ptr);
	void AllocateExecutableMemoryRegion_LuckTXM();
	ptrdiff_t AllocateWritableRegionAndGetDiff_LuckTXM();

	// LuckNoTXM
	void* AllocateExecutableMemory_LuckNoTXM(size_t size);
	void FreeExecutableMemory_LuckNoTXM(void* ptr, size_t size);
	ptrdiff_t AllocateWritableRegionAndGetDiff_LuckNoTXM(void* rx_ptr, size_t size);
	void FreeWritableRegion_LuckNoTXM(void* rx_ptr, size_t size, ptrdiff_t diff);

	// Legacy
	void* AllocateExecutableMemory_Legacy(size_t size);
	void FreeExecutableMemory_Legacy(void* ptr, size_t size);
	void JITPageWriteEnableExecuteDisable_Legacy(void* ptr);
	void JITPageWriteDisableExecuteEnable_Legacy(void* ptr);

#endif // TARGET_OS_IOS

} // namespace CodeGen
