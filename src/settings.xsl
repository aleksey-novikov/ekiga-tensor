<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text" />

  <xsl:template match="text()" />

  <!-- Accounts -->
  <xsl:template match="//sipProfile/sipLines">
    <xsl:text><![CDATA[gsettings set org.gnome.ekiga.protocols accounts '<?xml version="1.0"?><accounts>]]></xsl:text>
      <xsl:apply-templates select="line"/>
    <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line">
    <xsl:text><![CDATA[<account enabled="true" timeout="3600" type="SIP">]]></xsl:text>
      <xsl:apply-templates select="name|authName|authPassword|contact|proxy"/>
    <xsl:text><![CDATA[<roster/></account>]]></xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line/name">
    <xsl:text><![CDATA[<name>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</name>]]></xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line/contact">
    <xsl:text><![CDATA[<user>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</user>]]></xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line/proxy">
    <xsl:text><![CDATA[<host>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</host><outbound_proxy>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</outbound_proxy>]]></xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line/authName">
    <xsl:text><![CDATA[<auth_user>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</auth_user>]]></xsl:text>
  </xsl:template>

  <xsl:template match="//sipProfile/sipLines/line/authPassword">
    <xsl:text><![CDATA[<password>]]></xsl:text>
      <xsl:value-of select="." />
    <xsl:text><![CDATA[</password>]]></xsl:text>
  </xsl:template>

  <!-- Queue Settings -->
  <xsl:template match="//sbisofon/queuesettings">
      <xsl:text>gsettings set org.gnome.ekiga.queue enable </xsl:text>
      <xsl:if test="PauseQueueMember != '' and UnPauseQueueMember != ''">
      <xsl:text>true</xsl:text>
      </xsl:if>
      <xsl:if test="PauseQueueMember = '' or UnPauseQueueMember = ''">
      <xsl:text>false</xsl:text>
      </xsl:if>
      <xsl:text>&#10;</xsl:text>

      <xsl:text>gsettings set org.gnome.ekiga.queue enable-enter-leave </xsl:text>
      <xsl:if test="AddQueueMember != '' and RemoveQueueMember != ''">
      <xsl:text>true</xsl:text>
      </xsl:if>
      <xsl:if test="AddQueueMember = '' or RemoveQueueMember = ''">
      <xsl:text>false</xsl:text>
      </xsl:if>
      <xsl:text>&#10;</xsl:text>

      <xsl:apply-templates select="AddQueueMember|RemoveQueueMember|PauseQueueMember|UnPauseQueueMember" />
  </xsl:template>

  <xsl:template match="//sbisofon/queuesettings/AddQueueMember">
      <xsl:text>gsettings set org.gnome.ekiga.queue enter &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/queuesettings/RemoveQueueMember">
      <xsl:text>gsettings set org.gnome.ekiga.queue leave &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/queuesettings/PauseQueueMember">
      <xsl:text>gsettings set org.gnome.ekiga.queue pause &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/queuesettings/UnPauseQueueMember">
      <xsl:text>gsettings set org.gnome.ekiga.queue resume &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>


  <!-- Dial Prefixes -->
  <xsl:template match="//sbisofon/dialsettings">
      <xsl:apply-templates select="InternalPrefixLength|ExternalPrefixLength|ExternalPrefix|ExternalPefix|ExternalZonePrefix" />
  </xsl:template>

  <xsl:template match="//sbisofon/dialsettings/InternalPrefixLength">
      <xsl:text>gsettings set org.gnome.ekiga.general.call-options local-number-length &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/dialsettings/ExternalPrefixLength">
      <xsl:text>gsettings set org.gnome.ekiga.general.call-options outer-number-length &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/dialsettings/ExternalPrefix|//sbisofon/dialsettings/ExternalPefix">
      <xsl:text>gsettings set org.gnome.ekiga.general.call-options nonlocal-number-prefix &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="//sbisofon/dialsettings/ExternalZonePrefix">
      <xsl:text>gsettings set org.gnome.ekiga.general.call-options outer-number-prefix &apos;</xsl:text>
      <xsl:value-of select="." />
      <xsl:text>&apos;&#10;</xsl:text>
  </xsl:template>

</xsl:stylesheet>
