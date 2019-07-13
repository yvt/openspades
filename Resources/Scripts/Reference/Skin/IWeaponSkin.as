/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

namespace spades {

    /**
     * A skin of weapons. A class that implements
     * this might also have to implement either IThirdPersonToolSkin
     * or IViewToolSkin.
     */
    interface IWeaponSkin {
        /**
         * Receives a ready state. 0 = soon after firing,
         * >=1 = ready to fire the next bullet.
         */
        float ReadyState { set; }

        /** 0 = normal, 1 = aiming down the sight. */
        float AimDownSightState { set; }

        /** true if player is reloading a weapon */
        bool IsReloading { set; }

        /** 0 = reload has started, 1 = done */
        float ReloadProgress { set; }

        int Ammo { set; }
        int ClipSize { set; }

        /** Called when a player fired the weapon. */
        void WeaponFired();

        /**
         * Called when a player started reloading the weapon.
         * For shotgun, this is called for every pellets.
         */
        void ReloadingWeapon();

        /**
         * Called when a played reloaded the weapon.
         * For shotgun, this is called for every pellets.
         */
        void ReloadedWeapon();
    }

}
