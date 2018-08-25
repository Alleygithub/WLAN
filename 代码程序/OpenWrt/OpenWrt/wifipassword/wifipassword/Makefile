include $(TOPDIR)/rules.mk

PKG_NAME:=wifipassword
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wifipassword
	SECTION:=utils
	CATEGORY:=Utilties
	TITLE:=wifipassword
	DEPENDS:=libstdcpp
endef

define Build/Prepare
	$(MKDIR) -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/wifipassword/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wifipassword $(1)/bin
endef

$(eval $(call BuildPackage,wifipassword))
