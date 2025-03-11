#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class rvWeaponSniper : public rvWeapon {
public:

	CLASS_PROTOTYPE(rvWeaponSniper);

	rvWeaponSniper(void);

	virtual void		Spawn(void);
	virtual void		Think(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);
	void					PreSave(void);
	void					PostSave(void);

protected:

	float				spreadZoom;
	bool				fireHeld;

	bool				UpdateFlashlight(void);
	void				Flashlight(bool on);

private:

	stateResult_t		State_Idle(const stateParms_t& parms);
	stateResult_t		State_Fire(const stateParms_t& parms);
	stateResult_t		State_Reload(const stateParms_t& parms);
	stateResult_t		State_Flashlight(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvWeaponSniper);
};

CLASS_DECLARATION(rvWeapon, rvWeaponSniper)
END_CLASS

/*
================
rvWeaponSniper::rvWeaponSniper
================
*/
rvWeaponSniper::rvWeaponSniper(void) {
}

/*
================
rvWeaponSniper::Spawn
================
*/
void rvWeaponSniper::Spawn(void) {
	spreadZoom = spawnArgs.GetFloat("spreadZoom");
	fireHeld = false;

	SetState("Raise", 0);

	Flashlight(owner->IsFlashlightOn());
}

/*
================
rvWeaponSniper::Save
================
*/
void rvWeaponSniper::Save(idSaveGame* savefile) const {
	savefile->WriteFloat(spreadZoom);
	savefile->WriteBool(fireHeld);
}

/*
================
rvWeaponSniper::Restore
================
*/
void rvWeaponSniper::Restore(idRestoreGame* savefile) {
	savefile->ReadFloat(spreadZoom);
	savefile->ReadBool(fireHeld);
}

/*
================
rvWeaponSniper::PreSave
================
*/
void rvWeaponSniper::PreSave(void) {
}

/*
================
rvWeaponSniper::PostSave
================
*/
void rvWeaponSniper::PostSave(void) {
}


/*
================
rvWeaponSniper::Think
================
*/
void rvWeaponSniper::Think()
{
	rvWeapon::Think();
	if (zoomGui && owner == gameLocal.GetLocalPlayer()) {
		zoomGui->SetStateFloat("playerYaw", playerViewAxis.ToAngles().yaw);
	}
}

/*
================
rvWeaponSniper::UpdateFlashlight
================
*/
bool rvWeaponSniper::UpdateFlashlight(void) {
	if (!wsfl.flashlight) {
		return false;
	}

	SetState("Flashlight", 0);
	return true;
}

/*
================
rvWeaponSniper::Flashlight
================
*/
void rvWeaponSniper::Flashlight(bool on) {
	owner->Flashlight(on);

	if (on) {
		viewModel->ShowSurface("models/weapons/blaster/flare");
		worldModel->ShowSurface("models/weapons/blaster/flare");
	}
	else {
		viewModel->HideSurface("models/weapons/blaster/flare");
		worldModel->HideSurface("models/weapons/blaster/flare");
	}
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvWeaponSniper)
STATE("Idle", rvWeaponSniper::State_Idle)
STATE("Fire", rvWeaponSniper::State_Fire)
STATE("Reload", rvWeaponSniper::State_Reload)
STATE("Flashlight", rvWeaponSniper::State_Flashlight)
END_CLASS_STATES

/*
================
rvWeaponSniper::State_Idle
================
*/
stateResult_t rvWeaponSniper::State_Idle(const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch (parms.stage) {
	case STAGE_INIT:
		if (!AmmoAvailable()) {
			SetStatus(WP_OUTOFAMMO);
		}
		else {
			SetStatus(WP_READY);
		}

		PlayCycle(ANIMCHANNEL_ALL, "idle", parms.blendFrames);
		return SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		if (UpdateFlashlight()) {
			return SRESULT_DONE;
		}

		if (fireHeld && !wsfl.attack) {
			fireHeld = false;
		}
		if (!clipSize) {
			if (!fireHeld && gameLocal.time > nextAttackTime && wsfl.attack && AmmoAvailable()) {
				SetState("Fire", 0);
				return SRESULT_DONE;
			}
		}
		else {
			if (!fireHeld && gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip()) {
				SetState("Fire", 0);
				return SRESULT_DONE;
			}
			if (wsfl.attack && AutoReload() && !AmmoInClip() && AmmoAvailable()) {
				SetState("Reload", 4);
				return SRESULT_DONE;
			}
			if (wsfl.netReload || (wsfl.reload && AmmoInClip() < ClipSize() && AmmoAvailable() > AmmoInClip())) {
				SetState("Reload", 4);
				return SRESULT_DONE;
			}
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponSniper::State_Fire
================
*/
stateResult_t rvWeaponSniper::State_Fire(const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch (parms.stage) {
	case STAGE_INIT:
		if (wsfl.zoom) {
			nextAttackTime = gameLocal.time + (altFireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			Attack(true, 1, spreadZoom, 0, 1.0f);
			fireHeld = true;
		}
		else {
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			Attack(false, 1, spread, 0, 1.0f);
		}
		PlayAnim(ANIMCHANNEL_ALL, "fire", 0);
		return SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!fireHeld && wsfl.attack && gameLocal.time >= nextAttackTime && AmmoInClip() && !wsfl.lowerWeapon) {
			SetState("Fire", 0);
			return SRESULT_DONE;
		}
		if (AnimDone(ANIMCHANNEL_ALL, 0)) {
			SetState("Idle", 0);
			return SRESULT_DONE;
		}
		if (UpdateFlashlight()) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponSniper::State_Reload
================
*/
stateResult_t rvWeaponSniper::State_Reload(const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch (parms.stage) {
	case STAGE_INIT:
		if (wsfl.netReload) {
			wsfl.netReload = false;
		}
		else {
			NetReload();
		}

		SetStatus(WP_RELOAD);
		PlayAnim(ANIMCHANNEL_ALL, "reload", parms.blendFrames);
		return SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			AddToClip(ClipSize());
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvWeaponSniper::State_Flashlight
================
*/
stateResult_t rvWeaponSniper::State_Flashlight(const stateParms_t& parms) {
	enum {
		FLASHLIGHT_INIT,
		FLASHLIGHT_WAIT,
	};
	switch (parms.stage) {
	case FLASHLIGHT_INIT:
		SetStatus(WP_FLASHLIGHT);
		// Wait for the flashlight anim to play		
		PlayAnim(ANIMCHANNEL_ALL, "flashlight", 0);
		return SRESULT_STAGE(FLASHLIGHT_WAIT);

	case FLASHLIGHT_WAIT:
		if (!AnimDone(ANIMCHANNEL_ALL, 4)) {
			return SRESULT_WAIT;
		}

		if (owner->IsFlashlightOn()) {
			Flashlight(false);
		}
		else {
			Flashlight(true);
		}

		SetState("Idle", 4);
		return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}
