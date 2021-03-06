/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2013 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <osgEarth/ModelSource>
#include <osgEarth/Registry>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <OpenThreads/ScopedLock>

using namespace osgEarth;
using namespace OpenThreads;

/****************************************************************/


ModelSourceOptions::ModelSourceOptions( const ConfigOptions& options ) :
DriverConfigOptions( options ),
_minRange          ( 0.0f ),
_maxRange          ( FLT_MAX ),
_renderOrder       ( 11 ),
_depthTestEnabled  ( true )
{ 
    fromConfig(_conf);
}

ModelSourceOptions::~ModelSourceOptions()
{
}

void
ModelSourceOptions::fromConfig( const Config& conf )
{
    conf.getIfSet<float>( "min_range", _minRange );
    conf.getIfSet<float>( "max_range", _maxRange );
    conf.getIfSet<int>( "render_order", _renderOrder );
    conf.getIfSet<bool>( "depth_test_enabled", _depthTestEnabled );
}

void
ModelSourceOptions::mergeConfig( const Config& conf )
{
    DriverConfigOptions::mergeConfig( conf );
    fromConfig( conf );
}

Config
ModelSourceOptions::getConfig() const
{
    Config conf = DriverConfigOptions::getConfig();
    conf.updateIfSet( "min_range", _minRange );
    conf.updateIfSet( "max_range", _maxRange );
    conf.updateIfSet( "render_order", _renderOrder );
    conf.updateIfSet( "depth_test_enabled", _depthTestEnabled );
    return conf;
}

//------------------------------------------------------------------------

ModelSource::ModelSource( const ModelSourceOptions& options ) :
_options( options )
{
}

ModelSource::~ModelSource()
{
}


osg::Node* 
ModelSource::createNode(const Map*            map,
                        const osgDB::Options* dbOptions,
                        ProgressCallback*     progress )
{
    osg::Node* node = createNodeImplementation(map, dbOptions, progress);
    if ( node )
    {
        firePostProcessors( node );
    }
    return node;
}


void 
ModelSource::addPostProcessor( NodeOperation* op )
{
    if ( op )
    {
        Threading::ScopedMutexLock lock( _postProcessorsMutex );
        _postProcessors.push_back( op );
    }
}


void
ModelSource::removePostProcessor( NodeOperation* op )
{
    if ( op )
    {
        Threading::ScopedMutexLock lock( _postProcessorsMutex );
        NodeOperationVector::iterator i = std::find( _postProcessors.begin(), _postProcessors.end(), op );
        if ( i != _postProcessors.end() )
            _postProcessors.erase( i );
    }
}


void
ModelSource::firePostProcessors( osg::Node* node )
{
    if ( node )
    {
        Threading::ScopedMutexLock lock( _postProcessorsMutex );
        for( NodeOperationVector::iterator i = _postProcessors.begin(); i != _postProcessors.end(); ++i )
        {
            i->get()->operator()( node );
        }
    }
}

//------------------------------------------------------------------------

#undef  LC
#define LC "[ModelSourceFactory] "
#define MODEL_SOURCE_OPTIONS_TAG "__osgEarth::ModelSourceOptions"

ModelSourceFactory::~ModelSourceFactory()
{
}

ModelSource*
ModelSourceFactory::create( const ModelSourceOptions& options )
{
    ModelSource* modelSource = 0L;

    if ( !options.getDriver().empty() )
    {
        std::string driverExt = std::string(".osgearth_model_") + options.getDriver();

        osg::ref_ptr<osgDB::Options> rwopts = Registry::instance()->cloneOrCreateOptions();
        rwopts->setPluginData( MODEL_SOURCE_OPTIONS_TAG, (void*)&options );

        modelSource = dynamic_cast<ModelSource*>( osgDB::readObjectFile( driverExt, rwopts.get() ) );
        if ( modelSource )
        {
            //modelSource->setName( options.getName() );
            OE_INFO << "Loaded ModelSource driver \"" << options.getDriver() << "\" OK" << std::endl;
        }
        else
        {
            OE_WARN << "FAIL, unable to load model source driver for \"" << options.getDriver() << "\"" << std::endl;
        }
    }
    else
    {
        OE_WARN << LC << "FAIL, illegal null driver specification" << std::endl;
    }

    return modelSource;
}

//------------------------------------------------------------------------

const ModelSourceOptions&
ModelSourceDriver::getModelSourceOptions( const osgDB::ReaderWriter::Options* options ) const
{
    return *static_cast<const ModelSourceOptions*>( options->getPluginData( MODEL_SOURCE_OPTIONS_TAG ) );
}

ModelSourceDriver::~ModelSourceDriver()
{
}
